/**
* 
* @file
*
* @brief  FMOD FSB format parser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/archived/fmod.h"
//common includes
#include <byteorder.h>
//library includes
#include <binary/data_adapter.h>
#include <binary/input_stream.h>
#include <strings/format.h>

namespace Formats
{
namespace Archived
{
  namespace Fmod
  {
    namespace Ver5
    {
      class Format
      {
      public:
        explicit Format(const Binary::Container& rawData)
          : Stream(rawData)
          , Header(HeaderType::Read(Stream))
        {
        }

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
          //possibly truncated data
          const auto samplesData = Stream.ReadData(std::min(Header.SamplesDataSize, Stream.GetRestSize()));
          for (uint_t idx = 0; idx < Header.SamplesCount; ++idx)
          {
            target.StartSample(idx);
            const auto& loc = locations[idx];
            const auto nextOffset = locations[idx + 1].Offset;
            target.SetData(loc.SamplesCount, samplesData->GetSubcontainer(loc.Offset, nextOffset - loc.Offset));
          }
          return Stream.GetReadData();
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
            const auto sign = stream.ReadRawData(sizeof(SIGNATURE));
            Require(0 == std::memcmp(sign, SIGNATURE, sizeof(SIGNATURE)));
            const auto version = stream.ReadField<uint32_t>();
            HeaderType result;
            result.SamplesCount = fromLE(stream.ReadField<uint32_t>());
            result.SamplesHeadersSize = fromLE(stream.ReadField<uint32_t>());
            result.SamplesNamesSize = fromLE(stream.ReadField<uint32_t>());
            result.SamplesDataSize = fromLE(stream.ReadField<uint32_t>());
            result.Mode = fromLE(stream.ReadField<uint32_t>());
            stream.Skip(version == 0 ? 8 : 4);
            result.Flags = fromLE(stream.ReadField<uint32_t>());
            stream.Skip(16 + 8);//hash + dummy
            result.Size = stream.GetPosition();
            return result;
          }
        };
        
        class Metadata
        {
        public:
          Metadata(Binary::InputStream& stream, const HeaderType& hdr)
            : Stream(stream.ReadRawData(hdr.SamplesHeadersSize), hdr.SamplesHeadersSize)
          {
          }
          
          struct SampleLocation
          {
            std::size_t Offset;
            uint_t SamplesCount;
          };
          
          SampleLocation Read(Builder& target)
          {
            const auto header = fromLE(Stream.ReadField<uint64_t>());
            auto hasNextChunk = 0 != (header & 1);
            target.SetFrequency(DecodeSampleFrequency((header >> 1) & 15));
            target.SetChannels(DecodeChannelsCount(header));
            SampleLocation result;
            result.Offset = DecodeDataOffset(header);
            result.SamplesCount = DecodeSamplesCount(header);
            while (hasNextChunk)
            {
              const auto raw = fromLE(Stream.ReadField<uint32_t>());
              hasNextChunk = 0 != (raw & 1);
              const auto chunkSize = (raw >> 1) & 0xffffff;
              const auto chunkType = (raw >> 25) & 0x7f;
              target.AddMetaChunk(chunkType, Binary::DataAdapter(Stream.ReadRawData(chunkSize), chunkSize));
            }
            return result;
          }
        private:
          static uint_t DecodeSampleFrequency(uint_t id)
          {
            switch (id)
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
            return 1 + ((raw >> 5) & 1);
          }
          
          static std::size_t DecodeDataOffset(uint64_t raw)
          {
            return 16 * ((raw >> 6) & 0x0fffffff);
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
          Names(Binary::InputStream& stream, const HeaderType& hdr)
            : Stream(stream.ReadRawData(hdr.SamplesNamesSize), hdr.SamplesNamesSize)
            , HasNames(hdr.SamplesNamesSize != 0)
          {
          }
          
          String Read(uint_t idx)
          {
            if (HasNames)
            {
              Stream.Seek(idx * sizeof(uint32_t));
              Stream.Seek(fromLE(Stream.ReadField<uint32_t>()));
              return Stream.ReadCString(Stream.GetRestSize()).to_string();
            }
            else
            {
              return Strings::Format("#%1%", idx);
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
    }

    Binary::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      try
      {
        if (const auto result = Ver5::Format(rawData).Parse(target))
        {
          return result;
        }
      }
      catch (const std::exception&)
      {
      }
      return Binary::Container::Ptr();
    }
  }
}
}
