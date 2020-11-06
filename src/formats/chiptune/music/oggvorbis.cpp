/**
* 
* @file
*
* @brief  Ogg Vorbis parser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/music/oggvorbis.h"
#include "formats/chiptune/music/tags_vorbis.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <make_ptr.h>
//library includes
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <math/bitops.h>
#include <strings/encoding.h>
#include <strings/trim.h>
//std includes
#include <cctype>
#include <numeric>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace Ogg
  {
    const uint8_t SIGNATURE[] = {'O', 'g', 'g', 'S'};
    const uint8_t VERSION = 0;
    const uint64_t UNFINISHED_PAGE_POSITION = ~0ull;
    const uint_t MAX_SEGMENT_SIZE = 255;
    const uint_t MAX_PAGE_SIZE = 32768;

    //https://xiph.org/ogg/doc/framing.html
    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {
      }
      
      class Callback
      {
      public:
        virtual ~Callback() = default;
        
        virtual void OnStream(uint32_t streamId) = 0;
        virtual void OnPage(std::size_t offset, uint_t positionsCount, Binary::DataInputStream& payload) = 0;
      };
      
      Binary::Container::Ptr Parse(Callback& target)
      {
        static const std::size_t MIN_PAGE_SIZE = 27;
        uint32_t streamId = 0;
        uint32_t nextPageNumber = 0;
        uint64_t position = 0;
        while (Stream.GetRestSize() >= MIN_PAGE_SIZE)
        {
          const auto offset = Stream.GetPosition();
          //TODO: support seeking for pages
          if (0 != std::memcmp(Stream.PeekRawData(sizeof(SIGNATURE)), SIGNATURE, sizeof(SIGNATURE)))
          {
            return Container::Ptr();
          }
          Stream.Skip(sizeof(SIGNATURE));
          Require(VERSION == Stream.ReadByte());
          /*const auto flags = */Stream.ReadByte();
          const auto nextPosition = Stream.ReadLE<uint64_t>();
          const auto hasNextPosition = nextPosition != UNFINISHED_PAGE_POSITION;
          if (const auto stream = Stream.ReadLE<uint32_t>())
          {
            if (!streamId)
            {
              streamId = stream;
              target.OnStream(streamId);
            }
            else
            {
              //multiple streams are not suported
              Require(streamId == stream);
            }
          }
          Require(nextPageNumber++ == Stream.ReadLE<uint32_t>());
          /*const auto crc = */Stream.ReadLE<uint32_t>();
          const auto segmentsCount = Stream.ReadByte();
          const auto segmentsSizes = Stream.PeekRawData(segmentsCount);
          const auto payloadSize = std::accumulate(segmentsSizes, segmentsSizes + segmentsCount, std::size_t(0));
          {
            Stream.Skip(segmentsCount);
            Binary::DataInputStream payload(Stream.ReadData(payloadSize));
            target.OnPage(offset, hasNextPosition ? static_cast<uint_t>(nextPosition - position) : 0, payload);
          }
          if (hasNextPosition)
          {
            position = nextPosition;
          }
        }
        return Stream.GetReadContainer();
      }
    private:
      Binary::InputStream Stream;
    };

    class Builder
    {
    public:
      explicit Builder(std::size_t sizeHint)
        : Storage(sizeHint)
      {
      }

      void SetStreamId(uint32_t streamId)
      {
        StreamId = streamId;
      }

      void AddData(uint64_t position, const uint8_t* data, std::size_t size)
      {
        bool continued = false;
        while (size > MAX_PAGE_SIZE)
        {
          AddPage(UNFINISHED_PAGE_POSITION, Binary::View(data, MAX_PAGE_SIZE), continued);
          data += MAX_PAGE_SIZE;
          size -= MAX_PAGE_SIZE;
          continued = true;
        }
        AddPage(position, Binary::View(data, size), continued);
      }

      Binary::Container::Ptr CaptureResult()
      {
        auto& flags = Storage.Get<uint8_t>(LastPageOffset + 5);
        flags |= LAST_PAGE;
        CalculateCrc();
        return Storage.CaptureResult();
      }
    private:
      enum Flags
      {
        CONTINUED_PACKET = 1,
        FIRST_PAGE = 2,
        LAST_PAGE = 4
      };

      void AddPage(uint64_t position, Binary::View data, bool continued)
      {
        if (PagesDone)
        {
          CalculateCrc();
        }
        const uint8_t segmentsCount = static_cast<uint8_t>(data.Size() / MAX_SEGMENT_SIZE) + 1;
        LastPageOffset = Storage.Size();
        LastPageSize = 27 + segmentsCount + data.Size();
        Storage.Allocate(LastPageSize);
        Storage.Resize(LastPageOffset);
        Storage.Add(SIGNATURE);
        Storage.Add(VERSION);
        //assume single stream, so first page is always first
        const uint8_t flag = PagesDone == 0 ? FIRST_PAGE : (continued ? CONTINUED_PACKET : 0);
        Storage.Add(flag);
        Storage.Add(fromLE(position));
        Storage.Add(fromLE(StreamId));
        Storage.Add(fromLE(PagesDone++));
        const uint32_t EMPTY_CRC = 0;
        Storage.Add(EMPTY_CRC);
        Storage.Add(segmentsCount);
        WriteSegments(data.Size(), static_cast<uint8_t*>(Storage.Allocate(segmentsCount)));
        Storage.Add(data);
      }

      void WriteSegments(std::size_t size, uint8_t* data)
      {
        for (;;)
        {
          const auto part = std::min<uint_t>(MAX_SEGMENT_SIZE, size);
          *data++ = static_cast<uint8_t>(part);
          size -= part;
          if (!size)
          {
            if (part == MAX_SEGMENT_SIZE)
            {
              *data++ = 0;
            }
            break;
          }
        }
      }

      void CalculateCrc()
      {
        auto* const page = static_cast<uint8_t*>(Storage.Get(LastPageOffset));
        auto* const rawCrc = page + 22;
        std::memset(rawCrc, 0, sizeof(uint32_t));
        const auto crc = fromLE(Crc32::Calculate(page, LastPageSize));
        std::memcpy(rawCrc, &crc, sizeof(uint32_t));
      }

      class Crc32
      {
      public:
        Crc32()
        {
          const uint32_t CRC32_POLY = 0x04c11db7;
          for (uint_t i = 0; i < 256; ++i)
          {
            uint_t s = i << 24;
            for (uint_t j = 0; j < 8; ++j)
              s = (s << 1) ^ (s >= (1u << 31) ? CRC32_POLY : 0);
            Table[i] = s;
          }
        }

        static uint32_t Calculate(const uint8_t* data, std::size_t size)
        {
          static const Crc32 INSTANCE;
          uint32_t crc = 0;
          for (; size != 0; --size, ++data)
          {
            crc = (crc << 8) ^ INSTANCE.Table[*data ^ (crc >> 24)];
          }
          return crc;
        }
      private:
        uint32_t Table[256];
      };
    private:
      Binary::DataBuilder Storage;
      uint32_t StreamId = 0x2054585a;
      uint32_t PagesDone = 0;
      std::size_t LastPageOffset = 0;
      std::size_t LastPageSize = 0;
    };
  }
  
  //https://xiph.org/vorbis/doc/Vorbis_I_spec.html
  namespace Vorbis
  {
    const uint32_t VERSION = 0;
    const std::array<uint8_t, 6> SIGNATURE = {{'v', 'o', 'r', 'b', 'i', 's'}};

    enum PacketType
    {
      Identification = 1,
      Comment = 3,
      Setup = 5,
      Audio = 0 //arbitrary
    };

    class Format : public Ogg::Format::Callback
    {
    public:
      explicit Format(OggVorbis::Builder& target)
        : Target(target)
        , NextPacketType(Identification)
      {
      }

      void OnStream(uint32_t streamId) override
      {
        Target.SetStreamId(streamId);
      }

      void OnPage(std::size_t offset, uint_t positionsCount, Binary::DataInputStream& payload) override
      {
        while (payload.GetRestSize())
        {
          ReadPacket(offset, positionsCount, payload);
        }
      }
    private:
      void ReadPacket(std::size_t pageOffset, uint_t samplesCount, Binary::DataInputStream& payload)
      {
        if (NextPacketType != Audio)
        {
          const auto type = FindHeaderPacket(payload);
          Require(NextPacketType == type);
          ReadHeaderPacket(type, payload);
        }
        else
        {
          Target.AddFrame(pageOffset, samplesCount, payload.ReadRestData());
        }
      }
      
      static uint8_t FindHeaderPacket(Binary::DataInputStream& payload)
      {
        const auto avail = payload.GetRestSize();
        Require(avail > SIGNATURE.size() + 1);
        const auto raw = payload.PeekRawData(avail);
        const auto end = raw + avail;
        const auto sign = std::search(raw + 1, end, SIGNATURE.begin(), SIGNATURE.end());
        Require(sign != end);
        payload.Skip(sign + SIGNATURE.size() - raw);
        return sign[-1];
      }
      
      void ReadHeaderPacket(uint_t type, Binary::DataInputStream& payload)
      {
        switch (type)
        {
        case Identification:
          ReadIdentification(payload);
          NextPacketType = Comment;
          break;
        case Comment:
          ParseComment(payload, Target.GetMetaBuilder());
          NextPacketType = Setup;
          break;
        case Setup:
          payload.Seek(payload.GetPosition() - SIGNATURE.size() - 1);
          Target.SetSetup(payload.ReadRestData());
          NextPacketType = Audio;
          break;
        default:
          Require(false);
        }
      }
      
      void ReadIdentification(Binary::DataInputStream& payload)
      {
        const auto version = payload.ReadLE<uint32_t>();
        const auto channels = payload.ReadByte();
        const auto frequency = payload.ReadLE<uint32_t>();
        payload.Skip(4 * 3);
        const auto blocksize = payload.ReadByte();
        const auto framing = payload.ReadByte();
        const auto blockLo = blocksize & 0x0f;
        const auto blockHi = blocksize >> 4;
        Require(version == VERSION);
        Require(channels > 0);
        Require(frequency > 0);
        Require(blockLo >= 6 && blockLo <= 13);
        Require(blockHi >= 6 && blockHi <= 13);
        Require(blockLo <= blockHi);
        Require(framing & 1);
        Target.SetProperties(channels, frequency, 1 << blockLo, 1 << blockHi);
      }
    private:
      OggVorbis::Builder& Target;
      PacketType NextPacketType;
    };
  }
  
  namespace OggVorbis
  {
    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      try
      {
        Ogg::Format ogg(data);
        Vorbis::Format vorbis(target);
        if (const auto subData = ogg.Parse(vorbis))
        {
          return CreateCalculatingCrcContainer(subData, 0, subData->Size());
        }
        else
        {
          return Container::Ptr();
        }
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

      void SetStreamId(uint32_t /*streamId*/) override {}
      void SetProperties(uint_t /*channels*/, uint_t /*frequency*/, uint_t /*blockSizeLo*/, uint_t /*blockSizeHi*/) override {}
      void SetSetup(Binary::View /*data*/) override {}
      void AddFrame(std::size_t /*offset*/, uint_t /*samplesCount*/, Binary::View /*data*/) override {}
    };
    
    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    class SimpleDumpBuilder : public DumpBuilder
    {
    public:
      explicit SimpleDumpBuilder(std::size_t sizeHint)
        : Storage(sizeHint)
      {
      }

      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      
      void SetStreamId(uint32_t id) override
      {
        Storage.SetStreamId(id);
      }

      void SetProperties(uint_t channels, uint_t frequency, uint_t blockSizeLo, uint_t blockSizeHi) override
      {
        const auto blockSizes = (Math::Log2(blockSizeHi - 1) << 4) | (Math::Log2(blockSizeLo - 1));
        WriteIdentification(channels, frequency, blockSizes);
        WriteComment();
      }
      
      void SetSetup(Binary::View data) override
      {
        Storage.AddData(0, data.As<uint8_t>(), data.Size());
      }

      void AddFrame(std::size_t /*offset*/, uint_t framesCount, Binary::View data) override
      {
        TotalFrames += framesCount;
        Storage.AddData(TotalFrames, data.As<uint8_t>(), data.Size());
      }
      
      Binary::Container::Ptr GetDump() override
      {
        return Storage.CaptureResult();
      }
    private:
      void WriteIdentification(uint8_t channels, uint32_t frequency, uint8_t blockSizes)
      {
        Binary::DataBuilder builder(30);
        builder.Add(uint8_t(Vorbis::Identification));
        builder.Add(Vorbis::SIGNATURE);
        builder.Add(Vorbis::VERSION);
        builder.Add(channels);
        builder.Add(fromLE(frequency));
        builder.Allocate(3 * sizeof(uint32_t));
        builder.Add(blockSizes);
        builder.Add(uint8_t(1));
        assert(builder.Size() == 30);
        Storage.AddData(0, static_cast<const uint8_t*>(builder.Get(0)), builder.Size());
      }

      void WriteComment()
      {
        static const uint8_t DATA[] =
        {
          Vorbis::Comment,
          'v', 'o', 'r', 'b', 'i', 's',
          6, 0, 0, 0,
          'z', 'x', 't', 'u', 'n', 'e',
          0, 0, 0, 0,
          1
        };
        Storage.AddData(0, DATA, sizeof(DATA));
      }
    private:
      Ogg::Builder Storage;
      uint64_t TotalFrames = 0;
    };

    DumpBuilder::Ptr CreateDumpBuilder(std::size_t sizeHint)
    {
      return MakePtr<SimpleDumpBuilder>(sizeHint);
    }

    const StringView FORMAT =
      //first page
      "'O'g'g'S" //signature
      "00"       //version
      "02"       //flags, first page of logical bitstream
      "00{8}"    //position
      "?{4}"     //serial
      "00000000" //page
      "?{4}"     //crc
      "01 1e"    //1 lace for 30-bytes block size
        "01"           //identification
        "'v'o'r'b'i's" //signature
        "00{4}"        //version
        "01-02"        //mono/stereo supported
        "? ? 00-01 00" //up to 96kHz
        "?{12}"        //bitrate
        "66-dd"
        "%xxxxxxx1"    //frame sync
      "'O'g'g'S" //signature
      "00"       //version
      "00"       //flags
      "????00{4}"//first page may contain also audio data
      "?{4}"     //serial
      "01000000" //page
      "?{4}"     //crc
      "01-ff 01-ff" //more than one lace
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
        return Text::OGGVORBIS_DECODER_DESCRIPTION;
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
  } //namespace OggVorbis

  Decoder::Ptr CreateOGGDecoder()
  {
    return MakePtr<OggVorbis::Decoder>();
  }
}
}
