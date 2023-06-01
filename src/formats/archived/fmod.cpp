/**
 *
 * @file
 *
 * @brief  FMOD FSB format parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/fmod.h"
// common includes
#include <byteorder.h>
// library includes
#include <binary/input_stream.h>
#include <strings/format.h>

namespace Formats::Archived::Fmod
{
  namespace Ver5
  {
    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : Stream(rawData)
        , Header(HeaderType::Read(Stream))
      {}

      Binary::Container::Ptr Parse(Builder& target)
      {
        target.Setup(Header.SamplesCount, Header.Mode);
        Metadata meta(Stream, Header);
        Names names(Stream, Header);
        std::vector<Metadata::SampleLocation> locations;
        locations.reserve(Header.SamplesCount + 1);
        for (uint_t idx = 0; idx < Header.SamplesCount; ++idx)
        {
          target.StartSample(idx);
          target.SetName(names.Read(idx));
          locations.push_back(meta.Read(target));
        }
        locations.push_back({Header.SamplesDataSize, 0});
        // possibly truncated data
        const auto samplesData = Stream.ReadContainer(std::min(Header.SamplesDataSize, Stream.GetRestSize()));
        for (uint_t idx = 0; idx < Header.SamplesCount; ++idx)
        {
          target.StartSample(idx);
          const auto& loc = locations[idx];
          const auto nextOffset = locations[idx + 1].Offset;
          target.SetData(loc.SamplesCount, samplesData->GetSubcontainer(loc.Offset, nextOffset - loc.Offset));
        }
        return Stream.GetReadContainer();
      }

    private:
      struct HeaderType
      {
        std::size_t Size;
        uint_t SamplesCount;
        std::size_t SamplesHeadersSize;
        std::size_t SamplesNamesSize;
        std::size_t SamplesDataSize;
        uint_t Mode;
        uint_t Flags;

        static HeaderType Read(Binary::DataInputStream& stream)
        {
          static const uint8_t SIGNATURE[] = {'F', 'S', 'B', '5'};
          const auto sign = stream.ReadData(sizeof(SIGNATURE));
          Require(0 == std::memcmp(sign.Start(), SIGNATURE, sizeof(SIGNATURE)));
          const auto version = stream.Read<le_uint32_t>();
          HeaderType result;
          result.SamplesCount = stream.Read<le_uint32_t>();
          result.SamplesHeadersSize = stream.Read<le_uint32_t>();
          result.SamplesNamesSize = stream.Read<le_uint32_t>();
          result.SamplesDataSize = stream.Read<le_uint32_t>();
          result.Mode = stream.Read<le_uint32_t>();
          stream.Skip(version == 0 ? 8 : 4);
          result.Flags = stream.Read<le_uint32_t>();
          stream.Skip(16 + 8);  // hash + dummy
          result.Size = stream.GetPosition();
          return result;
        }
      };

      class Metadata
      {
      public:
        Metadata(Binary::DataInputStream& stream, const HeaderType& hdr)
          : Stream(stream.ReadData(hdr.SamplesHeadersSize))
        {}

        struct SampleLocation
        {
          std::size_t Offset;
          uint_t SamplesCount;
        };

        SampleLocation Read(Builder& target)
        {
          const uint64_t header = Stream.Read<le_uint64_t>();
          auto hasNextChunk = 0 != (header & 1);
          target.SetFrequency(DecodeSampleFrequency(header));
          target.SetChannels(DecodeChannelsCount(header));
          SampleLocation result;
          result.Offset = DecodeDataOffset(header);
          result.SamplesCount = DecodeSamplesCount(header);
          while (hasNextChunk)
          {
            const uint32_t raw = Stream.Read<le_uint32_t>();
            hasNextChunk = 0 != (raw & 1);
            const auto chunkSize = (raw >> 1) & 0xffffff;
            const auto chunkType = (raw >> 25) & 0x7f;
            target.AddMetaChunk(chunkType, Stream.ReadData(chunkSize));
          }
          return result;
        }

      private:
        // From low to high:
        // 1 bit flag
        // 4 bits frequency
        // 2 bits channels
        // 27 bits offset in blocks
        // 30 bits samples count

        static uint_t DecodeSampleFrequency(uint64_t raw)
        {
          switch ((raw >> 1) & 15)
          {
          case 0:
            return 4000;
          case 1:
            return 8000;
          case 2:
            return 11000;
          case 3:
            return 12000;
          case 4:
            return 16000;
          case 5:
            return 22050;
          case 6:
            return 24000;
          case 7:
            return 32000;
          case 8:
            return 44100;
          case 9:
            return 48000;
          case 10:
            return 96000;
          default:
            return 44100;
          }
        }

        static uint_t DecodeChannelsCount(uint64_t raw)
        {
          switch ((raw >> 5) & 3)
          {
          case 0:
            return 1;
          case 1:
            return 2;
          case 2:
            return 6;
          case 3:
            return 8;
          default:
            Require(false);
            return 1;
          }
        }

        static std::size_t DecodeDataOffset(uint64_t raw)
        {
          return 32 * ((raw >> 7) & 0x7ffffff);
        }

        static uint_t DecodeSamplesCount(uint64_t raw)
        {
          return raw >> 34;
        }

      private:
        Binary::DataInputStream Stream;
      };

      class Names
      {
      public:
        Names(Binary::DataInputStream& stream, const HeaderType& hdr)
          : Stream(stream.ReadData(hdr.SamplesNamesSize))
          , HasNames(hdr.SamplesNamesSize != 0)
        {}

        String Read(uint_t idx)
        {
          if (HasNames)
          {
            Stream.Seek(idx * sizeof(uint32_t));
            Stream.Seek(Stream.Read<le_uint32_t>());
            return Stream.ReadCString(Stream.GetRestSize()).to_string();
          }
          else
          {
            return Strings::Format("#{}", idx);
          }
        }

      private:
        Binary::DataInputStream Stream;
        const bool HasNames;
      };

    private:
      Binary::InputStream Stream;
      const HeaderType Header;
    };
  }  // namespace Ver5

  Binary::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
  {
    try
    {
      if (auto result = Ver5::Format(rawData).Parse(target))
      {
        return result;
      }
    }
    catch (const std::exception&)
    {}
    return {};
  }
}  // namespace Formats::Archived::Fmod
