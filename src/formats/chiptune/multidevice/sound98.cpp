/**
 *
 * @file
 *
 * @brief  S98 support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/multidevice/sound98.h"

#include <formats/chiptune/container.h>

#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <math/numeric.h>
#include <strings/casing.h>
#include <strings/sanitize.h>
#include <strings/trim.h>
#include <tools/range_checker.h>

#include <contract.h>
#include <make_ptr.h>
#include <string_view.h>

namespace Formats::Chiptune
{
  namespace Sound98
  {
    const auto DESCRIPTION = "Sound 98"sv;

    const uint32_t VER0 = 0x53393830;
    const uint32_t VER1 = 0x53393831;
    const uint32_t VER2 = 0x53393832;
    const uint32_t VER3 = 0x53393833;

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetTimings(Time::Milliseconds /*total*/, Time::Milliseconds /*loop*/) override {}
    };

    const std::size_t MIN_SIZE = 256;

    const auto FORMAT =
        "'S'9'8"    // signature
        "'0-'3"     // version
        "???00"     // mult
        "???00"     // div
        "00000000"  // not compressed
        "???00"     // offset to tag
        "??0000"    // offset to data
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
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

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (Format->Match(rawData))
        {
          return Parse(rawData, GetStubBuilder());
        }
        else
        {
          return {};
        }
      }

    private:
      const Binary::Format::Ptr Format;
    };

    namespace Tags
    {
      const uint8_t SIGNATURE[] = {'[', 'S', '9', '8', ']'};
      const auto TITLE = "title"sv;
      const auto ARTIST = "artist"sv;
      const auto GAME = "game"sv;
      const auto COMMENT = "comment"sv;

      bool Match(StringView str, StringView tag)
      {
        return Strings::EqualNoCaseAscii(str, tag);
      }
    }  // namespace Tags

    enum Areas
    {
      HEADER,
      DATA,
      DATA_END,
      TAG,
      TAG_END,
      END
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {}

      Formats::Chiptune::Container::Ptr Parse(Builder& target)
      {
        AreaController areas;
        areas.AddArea(END, Stream.GetRestSize());
        areas.AddArea(HEADER, 0);
        const uint_t sign = Stream.Read<be_uint32_t>();
        Require(Math::InRange(sign, VER0, VER3));
        const uint_t mult = Stream.Read<le_uint32_t>();
        const uint_t div = Stream.Read<le_uint32_t>();
        const uint_t compressed = Stream.Read<le_uint32_t>();
        // libvgm does not support compressed dumps
        Require(compressed == 0);
        if (const std::size_t tags = Stream.Read<le_uint32_t>())
        {
          areas.AddArea(TAG, tags);
        }
        areas.AddArea(DATA, Stream.Read<le_uint32_t>());
        const uint_t loopPoint = Stream.Read<le_uint32_t>();

        if (ParseTags(areas, sign, target.GetMetaBuilder()))
        {
          areas.AddArea(TAG_END, Stream.GetPosition());
        }
        if (ParseData(areas, loopPoint, mult ? mult : 10, div ? div : 1000, target))
        {
          areas.AddArea(DATA_END, Stream.GetPosition());
        }

        return CreateContainer(areas);
      }

    private:
      bool ParseTags(const AreaController& areas, uint_t sign, MetaBuilder& target)
      {
        const auto offset = areas.GetAreaAddress(TAG);
        if (offset == AreaController::Undefined || offset >= areas.GetAreaAddress(END))
        {
          return false;
        }
        Stream.Seek(offset);
        if (sign < VER3)
        {
          target.SetTitle(Strings::Sanitize(Stream.ReadCString(areas.GetAreaSize(TAG))));
          return true;
        }
        else if (ReadTagSignature())
        {
          const auto value = Strings::Sanitize(ReadMaybeTruncatedString(areas.GetAreaSize(TAG)));
          return ParsePSFTags(value, target);
        }
        else
        {
          return false;
        }
      }

      // Do not use ReadString - may be CR/LF inside
      StringView ReadMaybeTruncatedString(std::size_t maxSize)
      {
        const auto* start = Stream.PeekRawData(0);
        const auto* end = start + std::min(maxSize, Stream.GetRestSize());
        const auto* limit = std::find(start, end, 0);
        Stream.Skip(limit - start + (limit != end));
        return {safe_ptr_cast<const char*>(start), static_cast<std::size_t>(limit - start)};
      }

      bool ReadTagSignature()
      {
        static const uint8_t SIGN[] = {'[', 'S', '9', '8', ']'};
        if (const auto* sign = Stream.PeekRawData(sizeof(SIGN)))
        {
          if (0 == std::memcmp(sign, SIGN, sizeof(SIGN)))
          {
            Stream.Skip(sizeof(SIGN));
            return true;
          }
        }
        return false;
      }

      static bool ParsePSFTags(StringView val, MetaBuilder& target)
      {
        Binary::DataInputStream str(Binary::View(val.data(), val.size()));
        bool result = false;
        while (str.GetRestSize())
        {
          StringView name;
          StringView value;
          if (!ReadTagVariable(str, name, value))
          {
            // blank lines or invalid form
            continue;
          }
          else if (Tags::Match(name, Tags::TITLE))
          {
            target.SetTitle(Strings::Sanitize(value));
          }
          else if (Tags::Match(name, Tags::GAME))
          {
            target.SetProgram(Strings::Sanitize(value));
          }
          else if (Tags::Match(name, Tags::ARTIST))
          {
            target.SetAuthor(Strings::Sanitize(value));
          }
          else if (Tags::Match(name, Tags::COMMENT))
          {
            target.SetComment(Strings::SanitizeMultiline(value));
          }
          result = true;
        }
        return result;
      }

      static bool ReadTagVariable(Binary::DataInputStream& str, StringView& name, StringView& value)
      {
        const auto line = str.ReadString();
        const auto eqPos = line.find('=');
        if (eqPos != line.npos)
        {
          name = Strings::TrimSpaces(line.substr(0, eqPos));
          value = line.substr(eqPos + 1);
          return true;
        }
        else
        {
          return false;
        }
      }

      bool ParseData(const AreaController& areas, uint_t loopOffset, uint_t mult, uint_t div, Builder& target)
      {
        Stream.Seek(areas.GetAreaAddress(DATA));
        uint64_t totalTicks = 0;
        uint64_t loopTicks = 0;
        for (bool finished = false; !finished && Stream.GetRestSize();)
        {
          if (Stream.GetPosition() == loopOffset)
          {
            loopTicks = totalTicks;
          }
          switch (Stream.ReadByte())
          {
          case 0xff:
            ++totalTicks;
            break;
          case 0xfe:
            totalTicks += 2 + ReadVarInt();
            break;
          case 0xfd:
            finished = true;
            break;
          default:
            Stream.Skip(2);
            break;
          }
        }
        if (!totalTicks)
        {
          return false;
        }
        target.SetTimings(Time::Milliseconds::FromRatio(totalTicks * mult, div),
                          Time::Milliseconds::FromRatio(loopTicks * mult, div));
        return true;
      }

      uint_t ReadVarInt()
      {
        uint_t res = 0;
        for (uint_t shift = 0;; shift += 7)
        {
          const auto val = Stream.ReadByte();
          res |= uint_t(val & 0x7f) << shift;
          if (val < 0x80)
          {
            break;
          }
        }
        return res;
      }

      Formats::Chiptune::Container::Ptr CreateContainer(const AreaController& areas)
      {
        const auto dataOffset = areas.GetAreaAddress(DATA);
        const auto dataSize = areas.GetAreaSize(DATA);
        Require(dataSize != 0);

        const auto tagsEnd = areas.GetAreaAddress(TAG_END);
        const auto fileEnd = tagsEnd != AreaController::Undefined ? std::max(tagsEnd, dataOffset + dataSize)
                                                                  : dataOffset + dataSize;

        Stream.Seek(fileEnd);
        auto data = Stream.GetReadContainer();
        return CreateCalculatingCrcContainer(std::move(data), dataOffset, dataSize);
      }

    private:
      Binary::InputStream Stream;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      try
      {
        return Format(rawData).Parse(target);
      }
      catch (const std::exception&)
      {
        return {};
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }  // namespace Sound98

  Decoder::Ptr CreateSound98Decoder()
  {
    return MakePtr<Sound98::Decoder>();
  }
}  // namespace Formats::Chiptune
