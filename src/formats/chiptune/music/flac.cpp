/**
 *
 * @file
 *
 * @brief  Flac parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/music/flac.h"

#include "formats/chiptune/container.h"
#include "formats/chiptune/music/tags_id3.h"
#include "formats/chiptune/music/tags_vorbis.h"

#include "binary/format_factories.h"
#include "binary/input_stream.h"

#include "byteorder.h"
#include "make_ptr.h"

namespace Formats::Chiptune
{
  namespace Flac
  {
    const auto DESCRIPTION = "Free Lossless Audio Codec"sv;

    // https://www.xiph.org/flac/format
    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {}

      Container::Ptr Parse(Builder& target)
      {
        // Some of the tracks contain ID3 tag at the very beginning
        Id3::Parse(Stream, target.GetMetaBuilder());
        if (ParseSignature() && ParseMetadata(target) && ParseFrames(target))
        {
          if (const auto subData = Stream.GetReadContainer())
          {
            return CreateCalculatingCrcContainer(subData, 0, subData->Size());
          }
        }
        return {};
      }

    private:
      bool ParseSignature()
      {
        static const uint8_t SIGNATURE[] = {'f', 'L', 'a', 'C'};
        const auto* sign = Stream.PeekRawData(sizeof(SIGNATURE));
        if (sign && 0 == std::memcmp(sign, SIGNATURE, sizeof(SIGNATURE)))
        {
          Stream.Skip(sizeof(SIGNATURE));
          return true;
        }
        return false;
      }

      bool ParseMetadata(Builder& target)
      {
        const auto pos = Stream.GetPosition();
        while (Stream.PeekRawData(1 + sizeof(be_uint24_t)))
        {
          const auto flag = Stream.ReadByte();
          const bool isLast = flag & 128;
          const auto type = flag & 127;
          const uint_t payloadSize = Stream.Read<be_uint24_t>();
          if (type == 127 || Stream.GetRestSize() < payloadSize)
          {
            Stream.Seek(pos);
            break;
          }
          Binary::DataInputStream payload(Stream.ReadData(payloadSize));
          // See FLAC/format.h for FLAC_METADATA_TYPE_* enum
          if (type == 0)
          {
            ParseStreamInfo(payload, target);
          }
          else if (type == 4)
          {
            Vorbis::ParseComment(payload, target.GetMetaBuilder());
          }
          else if (type == 6)
          {
            ParsePicture(payload, target.GetMetaBuilder());
          }
          if (isLast)
          {
            return true;
          }
        }
        return false;
      }

      static void ParseStreamInfo(Binary::DataInputStream& input, Builder& target)
      {
        const uint_t minBlockSize = input.Read<be_uint16_t>();
        const uint_t maxBlockSize = input.Read<be_uint16_t>();
        target.SetBlockSize(minBlockSize, maxBlockSize);
        const auto minFrameSize = input.Read<be_uint24_t>();
        const auto maxFrameSize = input.Read<be_uint24_t>();
        target.SetFrameSize(minFrameSize, maxFrameSize);
        // big endian:
        // ffffffff ffffffff ffffcccb bbbbssss ssssssss ssssssss ssssssss ssssssss
        const uint64_t params = input.Read<be_uint64_t>();
        const auto totalSamples = params & 0xfffffffffuLL;
        const auto bitsPerSample = 1 + uint_t((params >> 36) & 0x1f);
        const auto channels = 1 + uint_t((params >> 41) & 7);
        const auto sampleRate = uint_t(params >> 44);
        Require(sampleRate != 0);
        target.SetStreamParameters(sampleRate, channels, bitsPerSample);
        target.SetTotalSamples(totalSamples);
      }

      static void ParsePicture(Binary::DataInputStream& input, MetaBuilder& target)
      {
        input.Skip(4);  // type
        const auto typeSize = input.Read<be_uint32_t>();
        input.Skip(typeSize);
        const auto descriptionSize = input.Read<be_uint32_t>();
        input.Skip(descriptionSize);
        input.Skip(4 * 4);  // width, height, depth, colors count
        const auto dataSize = input.Read<be_uint32_t>();
        target.SetPicture(input.ReadData(dataSize));
      }

      bool ParseFrames(Builder& target)
      {
        bool hasFrames = false;
        while (const auto headerSize = FindFrameHeader())
        {
          target.AddFrame(Stream.GetPosition());
          Stream.Skip(headerSize + FOOTER_SIZE);
          hasFrames = true;
        }
        if (hasFrames)
        {
          // make last frame till the end
          Stream.ReadRestData();
        }
        return hasFrames;
      }

      static const std::size_t MIN_HEADER_SIZE = 5;
      static const std::size_t MAX_HEADER_SIZE = 16;
      static const std::size_t FOOTER_SIZE = 2;

      std::size_t FindFrameHeader()
      {
        const auto limit = Stream.GetRestSize();
        const auto* const start = Stream.PeekRawData(limit);
        const auto* const end = start + limit;
        for (const auto* cursor = start; cursor + MAX_HEADER_SIZE < end;)
        {
          const auto headerSize = GetFrameHeaderSize(cursor);
          if (headerSize >= MIN_HEADER_SIZE)
          {
            Stream.Skip(cursor - start);
            return headerSize;
          }
          const auto* const match = std::find(cursor + 1, end, 0xff);
          if (match == end)
          {
            return 0;
          }
          cursor = match;
        }
        return false;
      }

      static std::size_t GetFrameHeaderSize(const uint8_t* hdr)
      {
        if (hdr[0] != 0xff || (hdr[1] & 0xf8) != 0xf8)
        {
          return 0;
        }
        const auto sampleRate = hdr[2] & 15;
        if (sampleRate == 0xf)
        {
          return 0;
        }
        uint_t crc = 0;
        for (std::size_t idx = 0; idx < MAX_HEADER_SIZE; ++idx)
        {
          crc ^= hdr[idx];
          for (uint_t bit = 0; bit < 8; ++bit)
          {
            crc <<= 1;
            if (crc & 0x100)
            {
              crc ^= 0x7;
            }
          }
          crc &= 0xff;
          if (crc == 0)
          {
            return idx;
          }
        }
        return 0;
      }

    private:
      Binary::InputStream Stream;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      try
      {
        return Format(data).Parse(target);
      }
      catch (const std::exception&)
      {
        return {};
      }
    }

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetBlockSize(uint_t /*minimal*/, uint_t /*maximal*/) override {}
      void SetFrameSize(uint_t /*minimal*/, uint_t /*maximal*/) override {}
      void SetStreamParameters(uint_t /*sampleRate*/, uint_t /*channels*/, uint_t /*bitsPerSample*/) override {}
      void SetTotalSamples(uint64_t /*count*/) override {}
      void AddFrame(std::size_t /*offset*/) override {}
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    const auto FORMAT =
        // ID3 tag    flac stream
        "'I         |'f"
        "'D         |'L"
        "'3         |'a"
        "00-04      |'C"
        "00-0a      |00"  // streaminfo metatag
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
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
  }  // namespace Flac

  Decoder::Ptr CreateFLACDecoder()
  {
    return MakePtr<Flac::Decoder>();
  }
}  // namespace Formats::Chiptune
