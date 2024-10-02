/**
 *
 * @file
 *
 * @brief  WAV parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/music/wav.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <strings/sanitize.h>
// std includes
#include <array>
#include <numeric>

namespace Formats::Chiptune
{
  namespace Wav
  {
    const Char DESCRIPTION[] = "Waveform Audio File Format";

    // http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    namespace Chunks
    {
      using Id = std::array<uint8_t, 4>;
      static const Id RIFF = {{'R', 'I', 'F', 'F'}};
      static const Id FMT = {{'f', 'm', 't', ' '}};
      static const Id FACT = {{'f', 'a', 'c', 't'}};
      static const Id DATA = {{'d', 'a', 't', 'a'}};
      static const Id LIST = {{'L', 'I', 'S', 'T'}};
    }  // namespace Chunks

    namespace Headers
    {
      static const std::array<uint8_t, 4> WAVE = {{'W', 'A', 'V', 'E'}};
      static const uint8_t INFO[] = {'I', 'N', 'F', 'O'};
    }  // namespace Headers

    class ChunksVisitor
    {
    public:
      virtual ~ChunksVisitor() = default;

      //! @return true if can accept more chunks
      virtual bool OnChunk(const Chunks::Id& id, Binary::Container::Ptr data) = 0;
      virtual void OnTruncatedChunk(const Chunks::Id& id, std::size_t declaredSize, Binary::Container::Ptr data) = 0;
    };

    Binary::Container::Ptr ParseChunks(const Binary::Container& data, ChunksVisitor& visitor)
    {
      Binary::InputStream stream(data);
      Chunks::Id id;
      static const std::size_t HEADER_SIZE = 8;
      while (const auto* const hdr = stream.PeekRawData(HEADER_SIZE))
      {
        std::memcpy(&id, hdr, sizeof(id));
        const auto size = ReadLE<uint32_t>(hdr + sizeof(id));
        stream.Skip(HEADER_SIZE);
        if (size == 0)
        {
          continue;
        }
        const auto avail = stream.GetRestSize();
        if (avail >= size)
        {
          if (visitor.OnChunk(id, stream.ReadContainer(size)))
          {
            continue;
          }
        }
        else if (avail != 0)
        {
          visitor.OnTruncatedChunk(id, size, stream.ReadContainer(avail));
        }
        break;
      }
      return stream.GetReadContainer();
    }

    class Parser : public ChunksVisitor
    {
    public:
      explicit Parser(Builder& target)
        : Target(target)
      {}

      bool OnChunk(const Chunks::Id& id, Binary::Container::Ptr data) override
      {
        if (id == Chunks::RIFF)
        {
          // simplify, do not track proper depth
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
        Require(0
                == std::memcmp(stream.ReadData(sizeof(Headers::WAVE)).Start(), Headers::WAVE.data(),
                               sizeof(Headers::WAVE)));
        ParseChunks(*stream.ReadRestContainer(), *this);
      }

      void ParseFormatChunk(Binary::View data)
      {
        Binary::DataInputStream stream(data);
        const uint_t formatTag = stream.Read<le_uint16_t>();
        const uint_t channels = stream.Read<le_uint16_t>();
        const uint_t frequency = stream.Read<le_uint32_t>();
        /*const auto dataRate = */ stream.Read<le_uint32_t>();
        const uint_t blockSize = stream.Read<le_uint16_t>();
        const uint_t bits = stream.Read<le_uint16_t>();
        Target.SetProperties(formatTag, frequency, channels, bits, blockSize);
        if (formatTag == Format::EXTENDED && stream.GetRestSize() >= 22)
        {
          const std::size_t extensionSize = stream.Read<le_uint16_t>();
          Require(extensionSize == stream.GetRestSize());
          const uint_t bitsOrBlockSize = stream.Read<le_uint16_t>();
          const uint_t channelsMask = stream.Read<le_uint32_t>();
          const auto formatId = stream.Read<Guid>();
          const auto rest = stream.ReadRestData();
          Target.SetExtendedProperties(bitsOrBlockSize, channelsMask, formatId, rest);
        }
        else if (stream.GetRestSize() > 2)
        {
          const std::size_t extensionSize = stream.Read<le_uint16_t>();
          Require(extensionSize == stream.GetRestSize());
          const auto rest = stream.ReadRestData();
          Target.SetExtraData(rest);
        }
      }

      void ParseSamplesCountHint(Binary::View data)
      {
        Require(data.Size() >= 4);
        Target.SetSamplesCountHint(ReadLE<uint32_t>(data.As<uint8_t>()));
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
            // do not fall parsing due to corrupted metadata
          }
        }
      }

    private:
      class MetaParser : public ChunksVisitor
      {
      public:
        explicit MetaParser(MetaBuilder& target)
          : Target(target)
        {}

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
          // TODO: parse comment
          return true;
        }

        void OnTruncatedChunk(const Chunks::Id& id, std::size_t /*declaredSize*/, Binary::Container::Ptr data) override
        {
          OnChunk(id, std::move(data));
        }

      private:
        static String ReadString(Binary::View data)
        {
          const StringView view(data.As<char>(), data.Size());
          return Strings::Sanitize(view);
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
        if (auto subData = ParseChunks(data, parser))
        {
          const auto totalSize = subData->Size();
          return CreateCalculatingCrcContainer(std::move(subData), 0, totalSize);
        }
      }
      catch (const std::exception&)
      {}
      return {};
    }

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetProperties(uint_t /*formatCode*/, uint_t /*frequency*/, uint_t /*channels*/, uint_t /*bits*/,
                         uint_t /*blocksize*/) override
      {}
      void SetExtendedProperties(uint_t /*validBitsOrBlockSize*/, uint_t /*channelsMask*/, const Guid& /*formatId*/,
                                 Binary::View /*restData*/) override
      {}
      void SetExtraData(Binary::View /*data*/) override {}
      void SetSamplesData(Binary::Container::Ptr /*data*/) override {}
      void SetSamplesCountHint(uint_t /*count*/) override {}
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    const auto FORMAT =
        "'R'I'F'F"
        "????"
        "'W'A'V'E"
        "'f'm't' "
        "10|12-ff 000000"  // chunk size up to 255 bytes
        "??"               // format code pcm/float
        "01|02 00"         // mono/stereo
        "?? 00-01 00"      // up to 96kHz
        "????"             // data rate
        "??"               // arbitraty block size
        "00|01-20 00"      // 1-32 bits per sample and 0 for special formats
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {}

      String GetDescription() const override
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
  }  // namespace Wav

  Decoder::Ptr CreateWAVDecoder()
  {
    return MakePtr<Wav::Decoder>();
  }
}  // namespace Formats::Chiptune
