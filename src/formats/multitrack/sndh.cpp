/**
 *
 * @file
 *
 * @brief  SNDH support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/multitrack/sndh.h"

#include "binary/container_base.h"
#include "binary/crc.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "formats/multitrack.h"
#include "strings/conversion.h"
#include "tools/locale_helpers.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"

#include <algorithm>
#include <utility>

namespace Formats::Multitrack
{
  namespace SNDH
  {
    using TagType = std::array<uint8_t, 4>;

    const TagType TAG_SNDH = {{'S', 'N', 'D', 'H'}};
    const TagType TAG_TITL = {{'T', 'I', 'T', 'L'}};
    const TagType TAG_COMM = {{'C', 'O', 'M', 'M'}};
    const TagType TAG_RIPP = {{'R', 'I', 'P', 'P'}};
    const TagType TAG_CONV = {{'C', 'O', 'N', 'V'}};
    const TagType TAG_YEAR = {{'Y', 'E', 'A', 'R'}};
    const TagType TAG_SUBTUNE_NAMES = {{'!', '#', 'S', 'N'}};
    const TagType TAG_TIME = {{'T', 'I', 'M', 'E'}};
    const TagType TAG_HDNS = {{'H', 'D', 'N', 'S'}};

    class StubBuilder : public Builder
    {
    public:
      Chiptune::MetaBuilder& GetMetaBuilder() override
      {
        return Chiptune::GetStubMetaBuilder();
      }

      void SetYear(StringView /*year*/) override {}
      void SetUnknownTag(StringView /*tag*/, StringView /*value*/) override {}

      void SetTimer(char /*type*/, uint_t /*freq*/) override {}
      void SetSubtunes(std::vector<StringView> /*names*/) override {}
      void SetDurations(std::vector<Time::Seconds> /*durations*/) override {}
    };

    const auto FORMAT =
        "????"      // init
        "????"      // exit
        "????"      // play
        "'S'N'D'H"  // signature
        ""sv;

    const auto DESCRIPTION = "Atari SNDH format"sv;

    struct Tracks
    {
      uint_t Count = 1;
      uint_t StartIndex = 0;
    };

    class Container : public Binary::BaseContainer<Multitrack::Container>
    {
    public:
      Container(Tracks trk, Binary::Container::Ptr data)
        : BaseContainer(std::move(data))
        , Trk(trk)
      {}

      uint_t FixedChecksum() const override
      {
        return Binary::Crc32(*Delegate);
      }

      uint_t TracksCount() const override
      {
        return Trk.Count;
      }

      uint_t StartTrackIndex() const override
      {
        return Trk.StartIndex;
      }

    private:
      const Tracks Trk;
    };

    class MetaFormat
    {
    public:
      explicit MetaFormat(Binary::View rawData)
        : Stream(rawData)
      {}

      void Parse(Builder& target)
      {
        while (const auto* tag = Stream.PeekField<TagType>())
        {
          if (*tag == TAG_HDNS)
          {
            break;
          }
          else if (ParseMetaTag(*tag, target.GetMetaBuilder(), target) || ParseTrack(*tag) || ParseTimers(*tag, target)
                   || ParseUnknownTag(*tag, target) || SkipPadding(*tag))
          {
            continue;
          }
          else
          {
            break;
          }
        }
      }

      Tracks GetTracks() const
      {
        return Trk;
      }

    private:
      bool ParseMetaTag(const TagType& tag, Chiptune::MetaBuilder& meta, Builder& target)
      {
        if (tag == TAG_TITL)
        {
          meta.SetTitle(ReadStringTag());
        }
        else if (tag == TAG_COMM)
        {
          meta.SetAuthor(ReadStringTag());
        }
        else if (tag == TAG_RIPP)
        {
          meta.SetComment(ReadStringTag());
        }
        else if (tag == TAG_CONV)
        {
          meta.SetProgram(ReadStringTag());
        }
        else if (tag == TAG_YEAR)
        {
          target.SetYear(ReadStringTag());
        }
        else if (tag == TAG_SUBTUNE_NAMES)
        {
          target.SetSubtunes(ReadSubtuneNames());
        }
        else if (tag == TAG_TIME)
        {
          target.SetDurations(ReadSubtuneDurations());
        }
        else
        {
          return false;
        }
        return true;
      }

      StringView ReadStringTag()
      {
        Stream.Skip(sizeof(TagType));
        return ReadCString();
      }

      std::vector<StringView> ReadSubtuneNames()
      {
        const auto offset = Stream.GetPosition();
        Stream.Skip(sizeof(TagType));
        std::vector<std::size_t> offsets(Trk.Count);
        for (auto& off : offsets)
        {
          off = Stream.Read<be_uint16_t>();
        }
        std::vector<StringView> result(Trk.Count);
        for (uint_t idx = 0; idx < Trk.Count; ++idx)
        {
          if (const auto off = offsets[idx])
          {
            Stream.Seek(offset + off);
            result[idx] = ReadCString();
          }
        }
        return result;
      }

      std::vector<Time::Seconds> ReadSubtuneDurations()
      {
        Stream.Skip(sizeof(TagType));
        std::vector<Time::Seconds> result(Trk.Count);
        for (auto& out : result)
        {
          out = Time::Seconds(Stream.Read<be_uint16_t>());
        }
        return result;
      }

      bool ParseTrack(const TagType& tag)
      {
        if (tag[0] == '#' && tag[1] == '#')
        {
          Stream.Skip(2);
          Trk.Count = ReadNumber();
        }
        else if (tag[0] == '!' && tag[1] == '#')
        {
          Stream.Skip(2);
          const auto num = ReadNumber();
          Require(num > 0);
          Trk.StartIndex = num - 1;
        }
        else
        {
          return false;
        }
        return true;
      }

      bool ParseTimers(const TagType& tag, Builder& target)
      {
        if ((tag[0] == 'T' && (tag[1] == 'A' || tag[1] == 'B' || tag[1] == 'C' || tag[1] == 'D'))
            || (tag[0] == '!' && tag[1] == 'V'))
        {
          Stream.Skip(2);
          target.SetTimer(tag[1], ReadNumber());
          return true;
        }
        else
        {
          return false;
        }
      }

      bool ParseUnknownTag(const TagType& tag, Builder& target)
      {
        if (IsAlpha(tag[0]) && IsAlpha(tag[1]) && IsAlpha(tag[2]) && IsAlpha(tag[3]))
        {
          const auto name = Stream.ReadData(sizeof(tag));
          target.SetUnknownTag({name.As<char>(), name.Size()}, ReadCString());
          return true;
        }
        else
        {
          return false;
        }
      }

      bool SkipPadding(const TagType& tag)
      {
        if (tag[0] == 0)
        {
          const auto avail = Stream.GetRestSize();
          const auto* start = Stream.PeekRawData(avail);
          const auto* data = std::find_if(start, start + avail, [](uint8_t b) { return b != 0; });
          Stream.Skip(data - start);
          return true;
        }
        else
        {
          return false;
        }
      }

      uint_t ReadNumber()
      {
        return Strings::ConvertTo<uint_t>(ReadCString());
      }

      StringView ReadCString()
      {
        return Stream.ReadCString(Stream.GetRestSize());
      }

    private:
      Binary::DataInputStream Stream;
      Tracks Trk;
    };

    struct RawHeader
    {
      uint8_t Table[12];
      TagType Signature;

      bool Matches() const
      {
        return Signature == TAG_SNDH;
      }

      // from psgplay
      std::size_t GetBranchTargetOffset(std::size_t offset) const
      {
        constexpr const std::size_t LIMIT = sizeof(Table);
        if (offset + sizeof(uint16_t) > LIMIT)
        {
          return 0;
        }
        const auto w1 = ReadBE<uint16_t>(Table + offset);
        if (w1 == 0x4e71)  // nop
        {
          return GetBranchTargetOffset(offset + sizeof(uint16_t));
        }
        else if (w1 == 0x6000 || w1 == 0x4efa)  // bra.w/jmp(pc)
        {
          return offset + 2 * sizeof(uint16_t) <= LIMIT
                     ? offset + sizeof(uint16_t) + ReadBE<uint16_t>(Table + offset + sizeof(uint16_t))
                     : 0;
        }
        else if ((w1 & 0xff00) == 0x6000)  // bra.s
        {
          return offset + (w1 & 0xff);
        }
        else
        {
          return 0;
        }
      }
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 16, "Invalid layout");

    Tracks ParseMeta(Binary::View rawData, Builder& target)
    {
      const auto* hdr = rawData.As<RawHeader>();
      Require(hdr->Matches());
      if (const auto metaLimit = std::min({rawData.Size(), hdr->GetBranchTargetOffset(0), hdr->GetBranchTargetOffset(4),
                                           hdr->GetBranchTargetOffset(8)}))
      {
        MetaFormat format(rawData.SubView(sizeof(*hdr), metaLimit));
        format.Parse(target);
        return format.GetTracks();
      }
      return {};
    }

    Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      try
      {
        return MakePtr<Container>(ParseMeta(rawData, target), rawData.GetSubcontainer(0, rawData.Size()));
      }
      catch (const std::exception&)
      {}
      return {};
    }

    class Decoder : public Formats::Multitrack::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {}

      StringView GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        return Parse(rawData, GetStubBuilder());
      }

    private:
      const Binary::Format::Ptr Format;
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }  // namespace SNDH

  Decoder::Ptr CreateSNDHDecoder()
  {
    return MakePtr<SNDH::Decoder>();
  }
}  // namespace Formats::Multitrack
