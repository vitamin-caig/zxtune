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
#include <binary/input_stream.h>
#include <binary/format_factories.h>
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
        
        virtual void OnPage(std::size_t offset, uint_t positionsCount, Binary::DataInputStream& payload) = 0;
      };
      
      Binary::Container::Ptr Parse(Callback& target)
      {
        static const std::size_t MIN_PAGE_SIZE = 27;
        static const uint_t VERSION = 0;
        static const uint8_t SIGNATURE[] = {'O', 'g', 'g', 'S'};
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
          if (const auto stream = Stream.ReadLE<uint32_t>())
          {
            if (!streamId)
            {
              streamId = stream;
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
          const auto segmentsSizes = Stream.ReadRawData(segmentsCount);
          const auto payloadSize = std::accumulate(segmentsSizes, segmentsSizes + segmentsCount, std::size_t(0));
          {
            Binary::DataInputStream payload(Stream.ReadRawData(payloadSize), payloadSize);
            target.OnPage(offset, static_cast<uint_t>(nextPosition - position), payload);
          }
          position = nextPosition;
        }
        return Stream.GetReadData();
      }
    private:
      static bool IsFreshPacket(uint8_t flag)
      {
        return 0 == (flag & 1);
      }
    private:
      Binary::InputStream Stream;
    };
  }
  
  //https://xiph.org/vorbis/doc/Vorbis_I_spec.html
  namespace Vorbis
  {
    class Format : public Ogg::Format::Callback
    {
    public:
      explicit Format(OggVorbis::Builder& target)
        : Target(target)
        , NextPacketType(Identification)
      {
      }

      void OnPage(std::size_t offset, uint_t positionsCount, Binary::DataInputStream& payload) override
      {
        while (payload.GetRestSize())
        {
          ReadPacket(offset, positionsCount, payload);
        }
      }
    private:
      enum PacketType
      {
        Identification = 1,
        Comment = 3,
        Setup = 5,
        Audio = 0 //arbitrary
      };
      
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
          Target.AddFrame(pageOffset, samplesCount);
          Skip(payload);
        }
      }
      
      static uint8_t FindHeaderPacket(Binary::DataInputStream& payload)
      {
        static const std::array<uint8_t, 6> SIGNATURE = {{'v', 'o', 'r', 'b', 'i', 's'}};
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
          Skip(payload);
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
        Require(version == 0);
        Require(channels > 0);
        Require(frequency > 0);
        Require(blockLo >= 6 && blockLo <= 13);
        Require(blockHi >= 6 && blockHi <= 13);
        Require(blockLo <= blockHi);
        Require(framing & 1);
        Target.SetChannels(channels);
        Target.SetFrequency(frequency);
      }
      
      void Skip(Binary::DataInputStream& payload)
      {
        const auto restSize = payload.GetRestSize();
        Require(restSize != 0);
        payload.Skip(restSize);
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

      void SetChannels(uint_t /*channels*/) override {}
      void SetFrequency(uint_t /*frequency*/) override {}
      void AddFrame(std::size_t /*offset*/, uint_t /*samplesCount*/) override {}
    };
    
    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
    
    const std::string FORMAT =
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
        "? 0f-bb 00 00"//8-48kHz
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
  } //namespace OggVorbis

  Decoder::Ptr CreateOGGDecoder()
  {
    return MakePtr<OggVorbis::Decoder>();
  }
}
}
