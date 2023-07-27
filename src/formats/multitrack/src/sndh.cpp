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

#include "formats/chiptune/common/container.h"

#include "binary/container_base.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "formats/multitrack/container.h"
#include "formats/multitrack/decoder.h"
#include "math/numeric.h"
#include "strings/conversion.h"
#include "tools/locale_helpers.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"

#include <algorithm>
#include <array>
#include <utility>

namespace Formats::Multitrack::SNDH
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
  const TagType TAG_FRAMES = {{'F', 'R', 'M', 'S'}};
  const TagType TAG_FLAGS = {{'F', 'L', 'A', 'G'}};
  const TagType TAG_HDNS = {{'H', 'D', 'N', 'S'}};

  // Some files declare more subsongs than really exist, but never that much
  constexpr uint_t MAX_SUBTUNES = 128;

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
    void SetFrames(std::vector<uint_t> /*frames*/) override {}
    void SetFlags(StringView /*flags*/) override {}
  };

  const auto FORMAT =
      "60 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ??"  // bra init/exit/play
      "'S'N'D'H"                             // signature
      ""sv;

  const auto DESCRIPTION = "Atari SNDH format"sv;

  struct Tracks
  {
    uint_t Count = 1;
    uint_t StartIndex = 0;
  };

  class Container : public Binary::BaseContainer<Multitrack::Container, Chiptune::Container>
  {
  public:
    Container(Tracks trk, Binary::Container::Ptr data)
      : BaseContainer(Chiptune::CreateCalculatingCrcContainer(*data))
      , Trk(trk)
    {}

    uint_t Checksum() const override
    {
      return Delegate->Checksum();
    }

    uint_t FixedChecksum() const override
    {
      return Delegate->FixedChecksum();
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
    MetaFormat(Binary::View rawData, uint_t subsongs)
      : Stream(rawData)
      , Trk{std::min(subsongs, MAX_SUBTUNES), 0}
    {}

    void Parse(Builder& target)
    {
      while (const auto* tag = Stream.PeekField<TagType>())
      {
        if (*tag == TAG_HDNS)
        {
          break;
        }
        else if (ParseMetaTag(*tag, target.GetMetaBuilder(), target) || ParseTrack(*tag) || ParseTimer(*tag, target)
                 || ParseNumbers(*tag, target) || ParseFlags(*tag, target) || ParseUnknownTag(*tag, target)
                 || SkipPadding(*tag))
        {
          continue;
        }
        else
        {
          // Unknown data: skip a single byte and try to resync on the next tag
          Stream.Skip(1);
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
      else
      {
        return false;
      }
      return true;
    }

    bool ParseTrack(const TagType& tag)
    {
      if (tag[0] == '#' && tag[1] == '#')
      {
        // ##NN: exactly two digits, sometimes followed by nul
        Stream.Skip(2);
        const auto count = ReadFixedDigNumber(2);
        if (Math::InRange<uint_t>(count, 1, MAX_SUBTUNES))
        {
          Trk.Count = count;
        }
      }
      else if (tag[0] == '!' && tag[1] == '#' && (tag[2] != 'S' || tag[3] != 'N'))
      {
        Stream.Skip(2);
        const auto num = ReadNumber();
        if (!Math::InRange<uint_t>(num, 1, Trk.Count))
        {
          return true;
        }
        Trk.StartIndex = num - 1;
      }
      else
      {
        return false;
      }
      return true;
    }

    bool ParseTimer(const TagType& tag, Builder& target)
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

    bool ParseNumbers(const TagType& tag, Builder& target)
    {
      if (tag == TAG_TIME)
      {
        Stream.Skip(sizeof(tag));
        std::vector<Time::Seconds> result(Trk.Count);
        for (auto& out : result)
        {
          out = Time::Seconds(Stream.Read<be_uint16_t>());
        }
        target.SetDurations(std::move(result));
      }
      else if (tag == TAG_FRAMES)
      {
        Stream.Skip(sizeof(tag));
        std::vector<uint_t> result(Trk.Count);
        for (auto& out : result)
        {
          out = Stream.Read<be_uint32_t>();
        }
        target.SetFrames(std::move(result));
      }
      else
      {
        return false;
      }
      return true;
    }

    bool ParseFlags(const TagType& tag, Builder& target)
    {
      if (tag != TAG_FLAGS)
      {
        return false;
      }
      Stream.Skip(sizeof(tag));
      // v2.2 FLAG value is '~' followed by nul-terminated list of flags
      if (const auto* tilde = Stream.PeekRawData(1); tilde && '~' == *tilde)
      {
        Stream.Skip(1);
        target.SetFlags(ReadCString());
      }
      else
      {
        target.SetFlags({});
      }
      return true;
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

    StringView ReadStringTag()
    {
      Stream.Skip(sizeof(TagType));
      return ReadCString();
    }

    std::vector<StringView> ReadSubtuneNames()
    {
      const auto base = Stream.GetPosition();
      Stream.Skip(sizeof(TagType));
      std::vector<std::size_t> offsets(Trk.Count);
      for (auto& off : offsets)
      {
        off = Stream.Read<be_uint16_t>();
      }
      std::vector<StringView> result(Trk.Count);
      std::size_t end = Stream.GetPosition();
      for (uint_t idx = 0; idx < Trk.Count; ++idx)
      {
        if (const auto off = offsets[idx])
        {
          Stream.Seek(base + off);
          result[idx] = ReadCString();
          end = std::max(end, Stream.GetPosition());
        }
      }
      Stream.Seek(end);
      return result;
    }

    uint_t ReadFixedDigNumber(std::size_t count)
    {
      const auto* const start = Stream.PeekRawData(count);
      if (!start)
      {
        return 0;
      }
      uint_t result = 0;
      for (std::size_t idx = 0; idx != count; ++idx)
      {
        const auto digit = start[idx];
        if (!IsDigit(char(digit)))
        {
          return 0;
        }
        result = result * 10 + (digit - '0');
      }
      Stream.Skip(count);
      return result;
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
      return Table[0] == 0x60 && Signature == TAG_SNDH;
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
        return offset + sizeof(uint16_t) + (w1 & 0xff);
      }
      else
      {
        return 0;
      }
    }
  };

  static_assert(sizeof(RawHeader) * alignof(RawHeader) == 16, "Invalid layout");

  const RawHeader* GetHeader(Binary::View rawData)
  {
    const auto* const hdr = rawData.As<RawHeader>();
    return hdr && hdr->Matches() ? hdr : nullptr;
  }

  // Subsong count may be declared after the data-dependent tags (e.g. FRMS),
  // so it is collected separately to make tags order-independent
  uint_t FindSubtunes(Binary::View tagsData)
  {
    uint_t result = 1;
    const auto* const data = tagsData.As<uint8_t>();
    const auto size = tagsData.Size();
    for (std::size_t pos = 0; pos + 4 <= size; ++pos)
    {
      if (data[pos] == '#' && data[pos + 1] == '#' && IsDigit(char(data[pos + 2])) && IsDigit(char(data[pos + 3])))
      {
        const uint_t count = 10 * (data[pos + 2] - '0') + (data[pos + 3] - '0');
        if (Math::InRange<uint_t>(count, 1, MAX_SUBTUNES))
        {
          result = count;
        }
        pos += 3;
      }
    }
    return result;
  }

  Tracks ParseMeta(Binary::View rawData, Builder& target)
  {
    const auto* const hdr = GetHeader(rawData);
    Require(hdr != nullptr);
    // Tags are stored between the header signature and the init entry point
    const auto initOffset = hdr->GetBranchTargetOffset(0);
    const auto tagsEnd = initOffset > sizeof(*hdr) ? std::min<std::size_t>(initOffset, rawData.Size()) : rawData.Size();
    const auto tagsData = rawData.SubView(sizeof(*hdr), tagsEnd - sizeof(*hdr));
    MetaFormat format(tagsData, FindSubtunes(tagsData));
    format.Parse(target);
    return format.GetTracks();
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
      return Format->Match(rawData) && GetHeader(rawData);
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
}  // namespace Formats::Multitrack::SNDH

namespace Formats::Multitrack
{
  Decoder::Ptr CreateSNDHDecoder()
  {
    return MakePtr<SNDH::Decoder>();
  }
}  // namespace Formats::Multitrack
