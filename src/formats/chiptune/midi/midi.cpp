/**
* 
* @file
*
* @brief  MIDI tracks format support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "midi.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
//library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <math/numeric.h>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::MIDI");
}

namespace Formats
{
namespace Chiptune
{
  namespace MIDI
  {
    typedef boost::array<uint8_t, 4> SignatureType;
    
    const SignatureType MTHD_SIGNATURE = {{'M', 'T', 'h', 'd'}};
    const SignatureType MTRK_SIGNATURE = {{'M', 'T', 'r', 'k'}};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RecordHeader
    {
      SignatureType Signature;
      uint32_t Size;
    } PACK_POST;
    
    PACK_PRE struct MThdRecord : RecordHeader
    {
      uint16_t Type;
      uint16_t NumTracks;
      int8_t TimeBaseHi;
      uint8_t TimeBaseLo;
    } PACK_POST;
    
    PACK_PRE struct MTrkRecord : RecordHeader
    {
      uint8_t Data[1];
    };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    const std::size_t MIN_SIZE = sizeof(MThdRecord) + sizeof(MTrkRecord) + 16;//average

    class StubBuilder : public Builder
    {
    public:
      virtual void SetTimeBase(TimeBase /*tickPeriod*/) {}
      virtual void SetTicksPerQN(uint_t /*ticks*/) {}
      virtual void SetTempo(TimeBase /*qnPeriod*/) {}
      virtual void StartTrack(uint_t /*idx*/) {}
      virtual void StartEvent(uint32_t /*ticksDelta*/) {}
      virtual void SetMessage(uint8_t /*status*/, uint8_t /*data*/) {}
      virtual void SetMessage(uint8_t /*status*/, uint8_t /*data1*/, uint8_t /*data2*/) {}
      virtual void SetMessage(const Dump& /*sysex*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void FinishTrack() {}
    };

    const std::string FORMAT(
      "'M'T'h'd"
      "00 00 00 06" //size
      "00 00|01" //type
      "00 01-10" //tracks
    );

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size = rawData.Size();
      if (size < MIN_SIZE)
      {
        return false;
      }
      const MThdRecord& hdr = *static_cast<const MThdRecord*>(rawData.Start());
      return hdr.Signature == MTHD_SIGNATURE
          && fromBE(hdr.Size) == 6
          && fromBE(hdr.Type) < 2
          && Math::InRange<uint_t>(fromBE(hdr.NumTracks), 1, 16);
    }

    
    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::MIDI_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };
    
    class InputStream : public Binary::InputStream
    {
    public:
      explicit InputStream(const Binary::Container& data)
        : Binary::InputStream(data)
      {
      }
      
      uint8_t ReadByte()
      {
        return *ReadData(1);
      }
      
      uint32_t ReadDword()
      {
        return fromBE(ReadField<uint32_t>());
      }
      
      uint32_t ReadVarInt()
      {
        uint32_t result = 0;
        for (uint_t idx = 0; ; ++idx)
        {
          Require(idx <= 3);
          const uint_t byte = ReadByte();
          result = (result << 7) | (byte & 0x7f);
          if (0 == (byte & 0x80))
          {
            break;
          }
        }
        return result;
      }
      
      Dump ReadBlob()
      {
        const uint_t size = ReadVarInt();
        const uint8_t* const data = ReadData(size);
        return Dump(data, data + size);
      }
      
      String ReadString()
      {
        const uint_t size = ReadVarInt();
        const uint8_t* const data = ReadData(size);
        return String(safe_ptr_cast<const char*>(data), size);
      }
    };
    
    //http://www.muzoborudovanie.ru/articles/midi/midi1.php
    class ModuleFormat
    {
    public:
      explicit ModuleFormat(const Binary::Container& data)
        : Data(data)
        , Stream(data)
        , Header(Stream.ReadField<MThdRecord>())
        , FixedStart(data.Size())
        , FixedEnd(0)
      {
      }
      
      uint_t GetTracksNumber() const
      {
        return fromBE(Header.NumTracks);
      }
      
      void ParseTimebase(Builder& builder)
      {
        if (IsSMPTETimeBase())
        {
          builder.SetTimeBase(GetSMPTETimeBase());
        }
        else
        {
          builder.SetTicksPerQN(GetTicksPerQN());
          builder.SetTempo(GetDefaultTempo());
        }
      }
      
      Binary::Container::Ptr GetNextTrack()
      {
        for (;;)
        {
          const RecordHeader& rec = Stream.ReadField<RecordHeader>();
          const std::size_t pos = Stream.GetPosition();
          const std::size_t size = fromBE(rec.Size);
          Stream.ReadData(size);
          if (rec.Signature == MTRK_SIGNATURE)
          {
            Dbg("Parse track at %1% with size %2%", pos, size);
            FixedStart = std::min(FixedStart, pos);
            FixedEnd = std::max(FixedEnd, pos + size);
            return Data.GetSubcontainer(pos, size);
          }
          else
          {
            Dbg("Skip unknown track at %1% with size %2%", pos, size);
          }
        }
      }
      
      std::size_t GetFixedStart() const
      {
        return FixedStart;
      }
      
      std::size_t GetFixedSize() const
      {
        return FixedEnd - FixedStart;
      }
      
      std::size_t GetUsedSize() const
      {
        return Stream.GetPosition();
      }
    private:
      bool IsSMPTETimeBase() const
      {
        return Header.TimeBaseHi < 0;
      }
      
      TimeBase GetSMPTETimeBase() const
      {
        const uint_t fps = -Header.TimeBaseHi;
        const uint_t sfpf = Header.TimeBaseLo;
        return Time::GetPeriodForFrequency<TimeBase>(fps * sfpf);
      }
      
      uint_t GetTicksPerQN() const
      {
        return 256 * Header.TimeBaseHi + Header.TimeBaseLo;
      }
      
      static TimeBase GetDefaultTempo()
      {
        //120 BPM
        return Time::Microseconds(500000);
      }
    private:
      const Binary::Container& Data;
      InputStream Stream;
      const MThdRecord Header;
      std::size_t FixedStart;
      std::size_t FixedEnd;
    };
    
    class TrackFormat
    {
    public:
      explicit TrackFormat(Binary::Container::Ptr data)
        : Data(data)
        , Stream(*Data)
      {
      }
      
      void Parse(Builder& builder)
      {
        for (uint_t runningStatus = 0; ;)
        {
          builder.StartEvent(Stream.ReadVarInt());
          const uint8_t data = Stream.ReadByte();
          if (IsSystemMessage(data))
          {
            runningStatus = 0;
            if (ParseSystemMessage(data, builder))
            {
              continue;
            }
            break;
          }
          if (IsStatusMessage(data))
          {
            runningStatus = data;
            ParseMessage(runningStatus, Stream.ReadByte(), builder);
          }
          else
          {
            ParseMessage(runningStatus, data, builder);
          }
        }
      }
    private:
      static bool IsSystemMessage(uint_t status)
      {
        return status >= 0xf0;
      }
      
      static bool IsStatusMessage(uint_t status)
      {
        return status >= 0x80;
      }
      
      enum SystemMessages
      {
        MSG_SYSEX = 0xf0,
        MSG_ESC = 0xf7,
        MSG_META = 0xff,
      };
      
      bool ParseSystemMessage(uint_t status, Builder& builder)
      {
        switch (status)
        {
        case MSG_SYSEX:
          builder.SetMessage(Stream.ReadBlob());
          break;
        case MSG_ESC:
          //skip
          Dbg("Skipping escaped sysex");
          Stream.ReadBlob();
          break;
        case MSG_META:
          return ParseMetaMessage(builder);
        default:
          Dbg("Unsupported event #%02x", status);
          break;
        }
        return true;
      }
      
      enum MetaMessages
      {
        MSG_SEQNUMBER = 0,
        MSG_TEXT = 1,
        MSG_COPYRIGHT = 2,
        MSG_TRACKNAME = 3,
        MSG_INSNAME = 4,
        MSG_LYRIC = 5,
        MSG_MARKER = 6,
        MSG_CUEPOINT = 7,
        MSG_PROGRAMNAME = 8,
        MSG_DEVICENAME = 9,
        MSG_CHANNELPREFIX = 0x20,
        MSG_PORT = 0x21,
        MSG_END = 0x2f,
        MSG_TEMPO = 0x51,
        MSG_SMPTEOFFSET = 0x54,
        MSG_TIMESIGNATURE = 0x58,
        MSG_KEYSIGNATURE = 0x59,
        MSG_SPECIFIC = 0x7f,
      };
      
      bool ParseMetaMessage(Builder& builder)
      {
        const uint_t metaType = Stream.ReadByte();
        switch (metaType)
        {
        case MSG_TRACKNAME:
          builder.SetTitle(Stream.ReadString());
          break;
        case MSG_END:
          builder.FinishTrack();
          return false;
        case MSG_TEMPO:
          {
            //ff 51 03 tt tt tt
            const uint32_t val = Stream.ReadDword();
            Require((val >> 24) == 3);
            const Time::Microseconds period(val & 0xffffff);
            builder.SetTempo(period);
          }
          break;
        default:
          Dbg("Unsupported meta message #%02x", metaType);
          Stream.ReadBlob();
          break;
        }
        return true;
      }
      
      void ParseMessage(uint_t status, uint8_t data, Builder& builder)
      {
        if (IsStatusMessage(status))
        {
          if ((status & 0xe0) == 0xc0)
          {
            builder.SetMessage(status, data);
          }
          else
          {
            builder.SetMessage(status, data, Stream.ReadByte());
          }
        }
        else
        {
          Dbg("Data message without previously defined status");
        }
      }
    private:
      const Binary::Container::Ptr Data;
      InputStream Stream;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }
      try
      {
        ModuleFormat module(data);
        module.ParseTimebase(target);
        for (uint_t trk = 0, lim = module.GetTracksNumber(); trk != lim; ++trk)
        {
          TrackFormat track(module.GetNextTrack());
          target.StartTrack(trk);
          track.Parse(target);
        }
        
        const std::size_t usedSize = module.GetUsedSize();
        const std::size_t fixedOffset = module.GetFixedStart();
        const std::size_t fixedSize = module.GetFixedSize();
        const Binary::Container::Ptr subData = data.GetSubcontainer(0, usedSize);
        return CreateCalculatingCrcContainer(subData, fixedOffset, fixedSize);
      }
      catch (const std::exception&)
      {
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace MIDI

  Decoder::Ptr CreateMIDIDecoder()
  {
    return boost::make_shared<MIDI::Decoder>();
  }
}//namespace Chiptune
}//namespace Formats
