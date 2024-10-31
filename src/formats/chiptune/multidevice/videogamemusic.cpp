/**
 *
 * @file
 *
 * @brief  VGM support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/multidevice/videogamemusic.h"

#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/container.h"

#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "binary/view.h"
#include "math/numeric.h"
#include "strings/array.h"
#include "strings/sanitize.h"
#include "strings/split.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <utility>

namespace Formats::Chiptune
{
  namespace VideoGameMusic
  {
    const auto DESCRIPTION = "Video Game Music"sv;

    const uint32_t SIGNATURE = 0x56676d20;

    const uint32_t GD3_SIGNATURE = 0x47643320;
    const uint_t VERSION_MIN = 100;
    const uint_t VERSION_MAX = 171;

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
        "'V'g'm' "  // signature
        "????"      // eof offset
        // version
        "00-09|10-19|20-29|30-39|40-49|50-59|60-69|70-71"
        "01 00 00"
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

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {}

      Formats::Chiptune::Container::Ptr Parse(Builder& target)
      {
        const auto limit = Stream.GetRestSize();
        Require(Stream.Read<be_uint32_t>() == SIGNATURE);
        // Support truncated files
        const auto eof = std::min(ReadOffset(), limit);
        const auto ver = ReadVersion(Stream.Read<le_uint32_t>());
        Require(Math::InRange(ver, VERSION_MIN, VERSION_MAX));
        Stream.Skip(8);
        const auto gd3Offset = ReadOffset();
        const uint_t totalSamples = Stream.Read<le_uint32_t>();
        Stream.Skip(4);
        const uint_t loopSamples = Stream.Read<le_uint32_t>();
        target.SetTimings(SamplesToTime(totalSamples), SamplesToTime(std::min(totalSamples, loopSamples)));
        const auto dataStart = GetDataOffset(ver);
        const uint_t NO_GD3 = 0x18;
        if (gd3Offset != NO_GD3 && gd3Offset < eof)
        {
          Stream.Seek(gd3Offset);
          if (!ParseTags(target.GetMetaBuilder()))
          {
            Stream.Seek(eof);
          }
        }
        else
        {
          Stream.Seek(eof);
        }
        const auto dataEnd = gd3Offset != NO_GD3 ? gd3Offset : eof;
        auto data = Stream.GetReadContainer();
        return CreateCalculatingCrcContainer(std::move(data), dataStart, dataEnd - dataStart);
      }

    private:
      static uint_t ReadVersion(uint32_t bcd)
      {
        Require(0 == (bcd & 0xffff0000));
        return Bcd(bcd & 0xff) + 100 * Bcd(bcd >> 8);
      }

      static uint_t Bcd(uint_t val)
      {
        const uint_t lo = val & 15;
        const uint_t hi = val >> 4;
        Require(lo < 10 && hi < 10);
        return lo + 10 * hi;
      }

      static Time::Milliseconds SamplesToTime(uint64_t samples)
      {
        const uint_t FREQUENCY = 44100;
        return Time::Milliseconds::FromRatio(samples, FREQUENCY);
      }

      std::size_t ReadOffset()
      {
        return Stream.GetPosition() + Stream.Read<le_uint32_t>();
      }

      uint_t GetDataOffset(uint_t ver)
      {
        if (ver < 150)
        {
          return 64;
        }
        else
        {
          const uint_t BASE = 0x34;
          Stream.Seek(BASE);
          return ReadOffset();
        }
      }

      bool ParseTags(MetaBuilder& target)
      {
        try
        {
          if (Stream.Read<be_uint32_t>() == GD3_SIGNATURE)
          {
            Stream.Skip(4);  // version
            const std::size_t size = Stream.Read<le_uint32_t>();
            ParseTags(Stream.ReadData(size), target);
            return true;
          }
        }
        catch (const std::exception&)
        {}
        return false;
      }

      static void ParseTags(Binary::View tags, MetaBuilder& target)
      {
        Binary::DataInputStream input(tags);
        const auto titleEn = ReadUTF16Sanitized(input);
        const auto titleJa = ReadUTF16Sanitized(input);
        target.SetTitle(DispatchString(titleEn, titleJa));
        const auto gameEn = ReadUTF16Sanitized(input);
        const auto gameJa = ReadUTF16Sanitized(input);
        target.SetProgram(DispatchString(gameEn, gameJa));
        /*const auto systemEn = */ ReadUTF16(input);
        /*const auto systemJa = */ ReadUTF16(input);
        const auto authorEn = ReadUTF16Sanitized(input);
        const auto authorJa = ReadUTF16Sanitized(input);
        /*const auto date = */ ReadUTF16(input);
        const auto ripper = ReadUTF16Sanitized(input);
        target.SetAuthor(DispatchString(authorEn, DispatchString(authorJa, ripper)));
        const auto comment = ReadUTF16(input);
        if (const auto splitted = Strings::Split(comment, "\r\n"sv); !splitted.empty())
        {
          Strings::Array strings(splitted.size());
          std::transform(splitted.begin(), splitted.end(), strings.begin(), &Strings::Sanitize);
          target.SetStrings(strings);
        }
      }

      static String ReadUTF16(Binary::DataInputStream& input)
      {
        String value;
        while (const uint16_t utf = input.Read<le_uint16_t>())
        {
          if (utf <= 0x7f)
          {
            value += static_cast<uint8_t>(utf);
          }
          else if (utf <= 0x7ff)
          {
            value += static_cast<uint8_t>(0xc0 | ((utf & 0x3c0) >> 6));
            value += static_cast<uint8_t>(0x80 | (utf & 0x3f));
          }
          else
          {
            value += static_cast<uint8_t>(0xe0 | ((utf & 0xf000) >> 12));
            value += static_cast<uint8_t>(0x80 | ((utf & 0x0fc0) >> 6));
            value += static_cast<uint8_t>(0x80 | ((utf & 0x003f)));
          }
        }
        return value;
      }

      static String ReadUTF16Sanitized(Binary::DataInputStream& input)
      {
        return Strings::Sanitize(ReadUTF16(input));
      }

      static const String& DispatchString(const String& lh, const String& rh)
      {
        return lh.empty() ? rh : lh;
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
  }  // namespace VideoGameMusic

  Decoder::Ptr CreateVideoGameMusicDecoder()
  {
    return MakePtr<VideoGameMusic::Decoder>();
  }
}  // namespace Formats::Chiptune
