/**
* 
* @file
*
* @brief  Flac parser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/music/flac.h"
#include "formats/chiptune/music/tags_id3.h"
#include "formats/chiptune/music/tags_vorbis.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <make_ptr.h>
//library includes
#include <binary/input_stream.h>
#include <binary/format_factories.h>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace Flac
  {
    //https://www.xiph.org/flac/format
    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {
      }
      
      Container::Ptr Parse(Builder& target)
      {
        //Some of the tracks contain ID3 tag at the very beginning
        Id3::Parse(Stream, target.GetMetaBuilder());
        if (ParseSignature() && ParseMetadata(target) && ParseFrames(target))
        {
          if (const auto subData = Stream.GetReadContainer())
          {
            return CreateCalculatingCrcContainer(subData, 0, subData->Size());
          }
        }
        return Container::Ptr();
      }
    private:
      bool ParseSignature()
      {
        static const uint8_t SIGNATURE[] = {'f', 'L', 'a', 'C'};
        if (0 == std::memcmp(Stream.PeekRawData(sizeof(SIGNATURE)), SIGNATURE, sizeof(SIGNATURE)))
        {
          Stream.Skip(sizeof(SIGNATURE));
          return true;
        }
        return false;
      }

      bool ParseMetadata(Builder& target)
      {
        static const std::size_t HEADER_SIZE = 4;
        while (const auto* hdr = Stream.PeekRawData(HEADER_SIZE))
        {
          const bool isLast = hdr[0] & 128;
          const auto type = hdr[0] & 127;
          const auto payloadSize = Byteorder<3>::ReadBE(hdr + 1);
          if (type == 127 || Stream.GetRestSize() < HEADER_SIZE + payloadSize)
          {
            break;
          }
          Stream.Skip(HEADER_SIZE);
          Binary::DataInputStream payload(Stream.ReadData(payloadSize));
          if (type == 0)
          {
            ParseStreamInfo(payload, target);
          }
          else if (type == 4)
          {
            Vorbis::ParseComment(payload, target.GetMetaBuilder());
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
        const auto minBlockSize = input.ReadBE<uint16_t>();
        const auto maxBlockSize = input.ReadBE<uint16_t>();
        target.SetBlockSize(minBlockSize, maxBlockSize);
        const auto minFrameSize = fromBE24(input.ReadData(3));
        const auto maxFrameSize = fromBE24(input.ReadData(3));
        target.SetFrameSize(minFrameSize, maxFrameSize);
        //TODO: operate with uint64_t
        const auto params = input.ReadData(8).As<uint8_t>();
        const auto sampleRate = (uint_t(params[0]) << 12) | (uint_t(params[1]) << 4) | (params[2] >> 4);
        Require(sampleRate != 0);
        const auto channels = 1 + ((params[2] >> 1) & 7);
        const auto bitsPerSample = 1 + (((params[2] & 1) << 4) | (params[3] >> 4));
        target.SetStreamParameters(sampleRate, channels, bitsPerSample);
        const auto totalSamples = (uint64_t(params[3] & 15) << 32) | (uint64_t(params[4]) << 24) | Byteorder<3>::ReadBE(params + 5);
        target.SetTotalSamples(totalSamples);
      }

      static inline uint_t fromBE24(Binary::View data)
      {
        return Byteorder<3>::ReadBE(data.As<uint8_t>());
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
        const auto start = Stream.PeekRawData(limit);
        const auto end = start + limit;
        for (auto cursor = start; cursor + MAX_HEADER_SIZE < end; )
        {
          const auto headerSize = GetFrameHeaderSize(cursor);
          if (headerSize >= MIN_HEADER_SIZE)
          {
            Stream.Skip(cursor - start);
            return headerSize;
          }
          const auto match = std::find(cursor + 1, end, 0xff);
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
        uint8_t crc = 0;
        for (std::size_t idx = 0; idx < MAX_HEADER_SIZE; ++idx)
        {
          crc ^= hdr[idx];
          for (uint_t bit = 0; bit < 8; ++bit)
          {
            crc = crc & 0x80 ? (crc << 1) ^ 0x7: crc << 1;
          }
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
        return Formats::Chiptune::Container::Ptr();
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
    
    const StringView FORMAT =
      //ID3 tag    flac stream
      "'I         |'f"
      "'D         |'L"
      "'3         |'a"
      "00-04      |'C"
      "00-0a      |00"//streaminfo metatag
    ;
    
    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {
      }

      String GetDescription() const override
      {
        return Text::FLAC_DECODER_DESCRIPTION;
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
          return Formats::Chiptune::Container::Ptr();
        }
      }
    private:
      const Binary::Format::Ptr Format;
    };
  } //namespace Flac

  Decoder::Ptr CreateFLACDecoder()
  {
    return MakePtr<Flac::Decoder>();
  }
}
}
