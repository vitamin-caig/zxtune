/**
* 
* @file
*
* @brief  WAV parser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "wav.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <make_ptr.h>
//library includes
#include <binary/data_builder.h>
#include <binary/input_stream.h>
#include <binary/format_factories.h>
#include <strings/encoding.h>
#include <strings/trim.h>
//std includes
#include <numeric>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace Wav
  {
    //http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    namespace Chunks
    {
      using Id = std::array<uint8_t, 4>;
      static const Id RIFF = {{'R', 'I', 'F', 'F'}};
      static const Id FMT = {{'f', 'm', 't', ' '}};
      static const Id FACT = {{'f', 'a', 'c', 't'}};
      static const Id DATA = {{'d', 'a', 't', 'a'}};
      static const Id LIST = {{'L', 'I', 'S', 'T'}};
    }
    
    namespace Headers
    {
      static const std::array<uint8_t, 4> WAVE = {{'W', 'A', 'V', 'E'}};
      static const uint8_t INFO[] = {'I', 'N', 'F', 'O'};
    }
    
    class ChunksVisitor
    {
    public:
      virtual ~ChunksVisitor() = default;
      
      //! @return true if can accept more chunks
      virtual bool OnChunk(const Chunks::Id& id, Binary::Container::Ptr data) = 0;
      virtual void OnTruncatedChunk(const Chunks::Id& id, std::size_t declaredSize, Binary::Container::Ptr data) = 0;
    };
    
    uint32_t ReadLE32(const uint8_t* data)
    {
      return (uint_t(data[3]) << 24) | (uint_t(data[2]) << 16) | (uint_t(data[1]) << 8) | data[0];
    }
    
    Binary::Container::Ptr ParseChunks(const Binary::Container& data, ChunksVisitor& visitor)
    {
      Binary::InputStream stream(data);
      Chunks::Id id;
      static const std::size_t HEADER_SIZE = 8;
      while (const auto hdr = stream.PeekRawData(HEADER_SIZE))
      {
        std::memcpy(&id, hdr, sizeof(id));
        const auto size = ReadLE32(hdr + sizeof(id));
        stream.Skip(HEADER_SIZE);
        if (size == 0)
        {
          continue;
        }
        const auto avail = stream.GetRestSize();
        if (avail >= size)
        {
          if (visitor.OnChunk(id, stream.ReadData(size)))
          {
            continue;
          }
        }
        else if (avail != 0)
        {
          visitor.OnTruncatedChunk(id, size, stream.ReadData(avail));
        }
        break;
      }
      return stream.GetReadData();
    }
    
    class Parser : public ChunksVisitor
    {
    public:
      explicit Parser(Builder& target)
        : Target(target)
      {
      }
      
      bool OnChunk(const Chunks::Id& id, Binary::Container::Ptr data) override
      {
        if (id == Chunks::RIFF)
        {
          //simplify, do not track proper depth
          ParseRiff(*data);
          return false;
        }
        else if (id == Chunks::FMT)
        {
          ParseFormatChunk(*data);
        }
        else if (id == Chunks::FACT)
        {
          ParseSamplesCountHint(*data);
        }
        else if (id == Chunks::DATA)
        {
          Target.SetSamplesData(std::move(data));
        }
        else if (id == Chunks::LIST)
        {
          ParseList(*data);
        }
        return true;
      }

      void OnTruncatedChunk(const Chunks::Id& id, std::size_t /*declaredSize*/, Binary::Container::Ptr data) override
      {
        Require(id != Chunks::FMT);
        OnChunk(id, std::move(data));
      }
      
    private:
      void ParseRiff(const Binary::Container& data)
      {
        Binary::InputStream stream(data);
        Require(0 == std::memcmp(stream.ReadRawData(sizeof(Headers::WAVE)), Headers::WAVE.data(), sizeof(Headers::WAVE)));
        ParseChunks(*stream.ReadRestData(), *this);
      }
      
      void ParseFormatChunk(const Binary::Container& data)
      {
        Binary::InputStream stream(data);
        auto formatTag = fromLE(stream.ReadField<uint16_t>());
        const auto channels = fromLE(stream.ReadField<uint16_t>());
        const auto frequency = fromLE(stream.ReadField<uint32_t>());
        /*const auto dataRate = */fromLE(stream.ReadField<uint32_t>());
        const auto blockSize = fromLE(stream.ReadField<uint16_t>());
        const auto bits = fromLE(stream.ReadField<uint16_t>());
        if (formatTag == 0xfffe)
        {
          const auto extensionSize = fromLE(stream.ReadField<uint16_t>());
          Require(extensionSize == stream.GetRestSize());
          Require(extensionSize == 22);
          stream.Skip(6);
          formatTag = fromLE(stream.ReadField<uint16_t>());
        }
        Target.SetProperties(formatTag, frequency, channels, bits, blockSize);
      }
      
      void ParseSamplesCountHint(const Binary::Container& data)
      {
        Require(data.Size() == 4);
        Target.SetSamplesCountHint(ReadLE32(static_cast<const uint8_t*>(data.Start())));
      }
      
      void ParseList(const Binary::Container& data)
      {
        if (data.Size() > sizeof(Headers::INFO) && 0 == std::memcmp(data.Start(), Headers::INFO, sizeof(Headers::INFO)))
        {
          MetaParser meta(Target.GetMetaBuilder());
          try
          {
            ParseChunks(*data.GetSubcontainer(sizeof(Headers::INFO), data.Size() - sizeof(Headers::INFO)), meta);
          }
          catch (const std::exception&)
          {
            //do not fall parsing due to corrupted metadata
          }
        }
      }
    private:
      class MetaParser : public ChunksVisitor
      {
      public:
        explicit MetaParser(MetaBuilder& target)
          : Target(target)
        {
        }

        bool OnChunk(const Chunks::Id& id, Binary::Container::Ptr data) override
        {
          static const Chunks::Id NAME = {{'I', 'N', 'A', 'M'}};
          static const Chunks::Id ARTIST = {{'I', 'A', 'R', 'T'}};
          if (id == NAME)
          {
            Target.SetTitle(ReadString(*data));
          }
          else if (id == ARTIST)
          {
            Target.SetAuthor(ReadString(*data));
          }
          //TODO: parse comment
          return true;
        }

        void OnTruncatedChunk(const Chunks::Id& id, std::size_t /*declaredSize*/, Binary::Container::Ptr data) override
        {
          OnChunk(id, std::move(data));
        }
      private:
        static String ReadString(const Binary::Data& data)
        {
          const StringView view(static_cast<const char*>(data.Start()), data.Size());
          return Strings::ToAutoUtf8(Strings::TrimSpaces(view));
        }
      private:
        MetaBuilder& Target;
      };
    private:
      Builder& Target;
    };
    
    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      try
      {
        Parser parser(target);
        if (const auto subData = ParseChunks(data, parser))
        {
          return CreateCalculatingCrcContainer(subData, 0, subData->Size());
        }
      }
      catch (const std::exception&)
      {
      }
      return Formats::Chiptune::Container::Ptr();
    }
    
    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetProperties(uint_t /*formatCode*/, uint_t /*frequency*/, uint_t /*channels*/, uint_t /*bits*/, uint_t /*blocksize*/) override {}
      void SetSamplesData(Binary::Container::Ptr /*data*/) override {}
      void SetSamplesCountHint(uint_t /*count*/) override {}
    };
    
    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
    
    class SimpleDumpBuilder : public DumpBuilder
    {
    public:
      SimpleDumpBuilder()
        : Storage(60)
      {
        Storage.Add(Chunks::RIFF);
        Storage.Add<uint32_t>(0);
        Storage.Add(Headers::WAVE);
        Storage.Add(Chunks::FMT);
        Storage.Add(fromLE<uint32_t>(16));
      }
      
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      
      void SetProperties(uint_t format, uint_t frequency, uint_t channels, uint_t bits, uint_t blockSize) override
      {
        Storage.Add(fromLE<uint16_t>(format));
        Storage.Add(fromLE<uint16_t>(channels));
        Storage.Add(fromLE<uint32_t>(frequency));
        Storage.Add(fromLE<uint32_t>(frequency * blockSize));
        Storage.Add(fromLE<uint16_t>(blockSize));
        Storage.Add(fromLE<uint16_t>(bits));
      }
      
      void SetSamplesData(Binary::Container::Ptr data) override
      {
        Storage.Add(Chunks::DATA);
        Storage.Add(fromLE<uint32_t>(data->Size()));
        Storage.Add(data->Start(), data->Size());
      }

      void SetSamplesCountHint(uint_t count) override
      {
        Storage.Add(Chunks::FACT);
        Storage.Add(fromLE<uint32_t>(4));
        Storage.Add(fromLE<uint32_t>(count));
      }
      
      Binary::Container::Ptr GetDump()
      {
        Storage.Get<uint32_t>(4) = fromLE(Storage.Size() - 8);
        return Storage.CaptureResult();
      }
    private:
      Binary::DataBuilder Storage;
    };

    DumpBuilder::Ptr CreateDumpBuilder()
    {
      return MakePtr<SimpleDumpBuilder>();
    }
    
    const std::string FORMAT =
      "'R'I'F'F"
      "????"
      "'W'A'V'E"
      "'f'm't' "
      "10|12-ff 000000" //chunk size up to 255 bytes
      "??"              //format code pcm/float
      "01|02 00"        //mono/stereo
      "?? 00-01 00"     //up to 96kHz
      "????"            //data rate
      "??"              //arbitraty block size
      "01-20 00"        //1-32 bits per sample
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
        return Text::WAV_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(const Binary::Container& rawData) const override
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
  } //namespace Wav

  Decoder::Ptr CreateWAVDecoder()
  {
    return MakePtr<Wav::Decoder>();
  }
}
}
