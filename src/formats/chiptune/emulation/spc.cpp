/**
* 
* @file
*
* @brief  SPC support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/emulation/spc.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/chiptune.h>
#include <strings/optimize.h>
//std includes
#include <array>
//boost includes
#include <boost/lexical_cast.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace SPC
  {
    const Debug::Stream Dbg("Formats::Chiptune::SPC");

    typedef std::array<uint8_t, 28> SignatureType;
    const SignatureType SIGNATURE = {{
      'S', 'N', 'E', 'S', '-', 'S', 'P', 'C', '7', '0', '0', ' ',
      'S', 'o', 'u', 'n', 'd', ' ', 'F', 'i', 'l', 'e', ' ', 'D', 'a', 't', 'a', ' '
    }};
    
    inline String GetString(const char* begin, const char* end)
    {
      return String(begin, std::find(begin, end, '\0'));
    }
    
    template<std::size_t D>
    inline String GetString(const char (&str)[D])
    {
      return GetString(str, str + D);
    }
    
    inline bool IsValidDate(const String& str)
    {
      return String::npos == str.find_first_not_of("0123456789/-");
    }
    
    inline bool IsValidDigits(const String& str)
    {
      return String::npos == str.find_first_not_of("0123456789");
    }
    
    inline String DateFromInteger(uint_t val)
    {
      String result(8, ' ');
      for (char & pos : result)
      {
        pos = '0' + (val & 15);
        val >>= 4;
      }
      return result;
    }

    inline uint_t ToInt(const String& str)
    {
      if (str.empty())
      {
        return 0;
      }
      else if (IsValidDigits(str))
      {
        return boost::lexical_cast<uint_t>(str);
      }
      else
      {
        return ~uint_t(0);
      }
    }
    
    using Tick = Time::BaseUnit<uint_t, 64000>;
    
    const auto MAX_DURATION = Time::Seconds(3600);
    
    template<class T>
    inline bool IsValidTime(T t)
    {
      return t < MAX_DURATION;
    }
    
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Registers
    {
      uint16_t PC;
      uint8_t A;
      uint8_t X;
      uint8_t Y;
      uint8_t PSW;
      uint8_t SP;
      uint16_t Reserved;
    } PACK_POST;
    
    PACK_PRE struct ID666TextTag
    {
      char Song[32];
      char Game[32];
      char Dumper[16];
      char Comments[32];
      char DumpDate[11];//MM/DD/YYYY or MM-DD-YYY or empty???
      char FadeTimeSec[3];
      char FadeDurationMs[5];
      char Artist[32];
      uint8_t DisableDefaultChannel;//1-do
      uint8_t Emulator;
      uint8_t Reserved[45];
      
      String GetDumpDate() const
      {
        return GetString(DumpDate);
      }
      
      Time::Seconds GetFadeTime() const
      {
        const String& str = GetString(FadeTimeSec);
        const uint_t val = ToInt(str);
        return Time::Seconds(val);
      }
      
      Time::Milliseconds GetFadeDuration() const
      {
        const String& str = GetString(FadeDurationMs);
        const uint_t val = ToInt(str);
        return Time::Milliseconds(val);
      }
    } PACK_POST;
    
    PACK_PRE struct ID666BinTag
    {
      char Song[32];
      char Game[32];
      char Dumper[16];
      char Comments[32];
      uint32_t DumpDate;//YYYYMMDD
      uint8_t Unused[7];
      uint8_t FadeTimeSec[3];
      uint32_t FadeDurationMs;
      char Artist[32];
      uint8_t DisableDefaultChannel;//1-do
      uint8_t Emulator;
      uint8_t Reserved[46];
      
      String GetDumpDate() const
      {
        return DateFromInteger(fromLE(DumpDate));
      }

      Time::Seconds GetFadeTime() const
      {
        const uint_t val = uint_t(FadeTimeSec[0]) | (uint_t(FadeTimeSec[1]) << 8) | (uint_t(FadeTimeSec[2]) << 16);
        return Time::Seconds(val);
      }
      
      Time::Milliseconds GetFadeDuration() const
      {
        return Time::Milliseconds(FadeDurationMs);
      }
    } PACK_POST;
    
    PACK_PRE struct ExtraRAM
    {
      //usually at +0x10180
      uint8_t Unused[64];
      uint8_t Data[64];
    } PACK_POST;
    
    PACK_PRE struct RawHeader
    {
      SignatureType Signature;
      uint8_t VersionString[5];//vX.XX or 0.10\00
      uint8_t Padding[2];//26,26
      uint8_t UseID666;//26-no, 27- yes, not used
      uint8_t VersionMinor;
      //+0x25
      Registers Regs;
      //+0x2e
      union ID666Tag
      {
        ID666TextTag TextTag;
        ID666BinTag BinTag;
      } ID666;
      //+0x100
      uint8_t RAM[65536];
      //+0x10100
      uint8_t DSPRegisters[128];
    } PACK_POST;
    
    typedef std::array<uint8_t, 4> IFFId;
    const IFFId XID6 = {{'x', 'i', 'd', '6'}};

    PACK_PRE struct IFFChunkHeader
    {
      IFFId ID;//xid6
      uint32_t DataSize;
    } PACK_POST;
    
    PACK_PRE struct SubChunkHeader
    {
      uint8_t ID;
      uint8_t Type;
      uint16_t DataSize;
      
      enum Types
      {
        Length = 0,//in DataSize
        Asciiz = 1,//ASCIIZ
        Integer = 4//ui32LE
      };
      
      enum IDs
      {
        //Asciiz
        SongName = 1,
        GameName,
        ArtistName,
        DumperName,
        //Integer
        Date,
        //Length
        Emulator,
        //Asciiz
        Comments,
        OfficialTitle = 16,
        //Length
        OSTDisk,
        OSTTrack,
        //Asciiz
        PublisherName,
        //Length
        CopyrightYear,
        //Integer
        IntroductionLength = 48,
        LoopLength,
        EndLength,
        FadeLength,
        //Length
        MutedChannels,
        LoopTimes,
        //Integer
        Amplification,
      };
      
      uint_t GetDataSize() const
      {
        return Type != Length ? fromLE(DataSize) : 0;
      }
      
      uint_t GetInteger() const
      {
        if (Type == Length)
        {
          return fromLE(DataSize);
        }
        else if (Type == Integer)
        {
          return fromLE(*safe_ptr_cast<const uint32_t*>(this + 1));
        }
        else
        {
          assert(!"Invalid subchunk type");
          return 0;
        }
      }
      
      Time::Milliseconds GetTicks() const
      {
        return Time::Duration<Tick>(GetInteger()).CastTo<Time::Millisecond>();
      }
      
      String GetString() const
      {
        if (Type == Asciiz)
        {
          const char* const start = safe_ptr_cast<const char*>(this + 1);
          const char* const end = start + fromLE(DataSize);
          const StringView val = end[-1] ? StringView(start, end) : StringView(start);
          return Strings::OptimizeAscii(val);
        }
        else
        {
          //assert(!"Invalid subchunk type");
          return String();
        }
      }
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
    
    static_assert(sizeof(Registers) == 9, "Invalid layout");
    static_assert(sizeof(ID666TextTag) == 0xd2, "Invalid layout");
    static_assert(sizeof(ID666BinTag) == 0xd2, "Invalid layout");
    static_assert(sizeof(RawHeader) == 0x10180, "Invalid layout");
    static_assert(sizeof(ExtraRAM) == 0x80, "Invalid layout");
    static_assert(sizeof(IFFChunkHeader) == 8, "Invalid layout");
    static_assert(sizeof(SubChunkHeader) == 4, "Invalid layout");

    class StubBuilder : public Builder
    {
    public:
      void SetRegisters(uint16_t /*pc*/, uint8_t /*a*/, uint8_t /*x*/, uint8_t /*y*/, uint8_t /*psw*/, uint8_t /*sp*/) override {}
      void SetTitle(String /*title*/) override {}
      void SetGame(String /*game*/) override {}
      void SetDumper(String /*dumper*/) override {}
      void SetComment(String /*comment*/) override {}
      void SetDumpDate(String /*date*/) override {}
      void SetIntro(Time::Milliseconds /*duration*/) override {}
      void SetLoop(Time::Milliseconds /*duration*/) override {}
      void SetFade(Time::Milliseconds /*duration*/) override {}
      void SetArtist(String /*artist*/) override {}
      
      void SetRAM(Binary::View /*data*/) override {}
      void SetDSPRegisters(Binary::View /*data*/) override {}
      void SetExtraRAM(Binary::View /*data*/) override {}
    };
    
    //used nes_spc library doesn't support another versions
    const std::string FORMAT(
      "'S'N'E'S'-'S'P'C'7'0'0' 'S'o'u'n'd' 'F'i'l'e' 'D'a't'a' "
      //actual|old
      "'v     |'0"
      "'0     |'."
      "'.     |'1"
      "('1-'3)|'0"
      "('0-'9)|00"
      "1a     |00"
      "1a     |00"
      "1a|1b  |00"              //has ID666
      "0a-1e  |00"              //version minor
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, sizeof(RawHeader)))
      {
      }

      String GetDescription() const override
      {
        return Text::SPC_DECODER_DESCRIPTION;
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
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };
    
    struct Tag
    {
      const String Song;
      const String Game;
      const String Dumper;
      const String Comments;
      const String DumpDate;
      const Time::Seconds FadeTime;
      const Time::Milliseconds FadeDuration;
      const String Artist;
      
      template<class T>
      explicit Tag(const T& tag)
        : Song(GetString(tag.Song))
        , Game(GetString(tag.Game))
        , Dumper(GetString(tag.Dumper))
        , Comments(GetString(tag.Comments))
        , DumpDate(tag.GetDumpDate())
        , FadeTime(tag.GetFadeTime())
        , FadeDuration(tag.GetFadeDuration())
        , Artist(GetString(tag.Artist))
      {
      }
      
      uint_t GetScore() const
      {
        return Song.size()
             + Game.size()
             + Dumper.size()
             + Comments.size()
             + DumpDate.size() * IsValidDate(DumpDate)
             + 100 * IsValidTime(FadeTime)
             + 100 * IsValidTime(FadeDuration)
        ;
      }
    };
    
    class Format
    {
    public:
      explicit Format(Binary::View data)
        : Stream(data)
      {
      }
      
      void ParseMainPart(Builder& target)
      {
        const RawHeader& hdr = Stream.ReadField<RawHeader>();
        Require(hdr.Signature == SIGNATURE);
        ParseID666(hdr.ID666, target);
        target.SetRegisters(fromLE(hdr.Regs.PC), hdr.Regs.A, hdr.Regs.X, hdr.Regs.Y, hdr.Regs.PSW, hdr.Regs.SP);
        target.SetRAM(hdr.RAM);
        target.SetDSPRegisters(hdr.DSPRegisters);
        if (Stream.GetRestSize() >= sizeof(ExtraRAM))
        {
          const ExtraRAM& extra = Stream.ReadField<ExtraRAM>();
          target.SetExtraRAM(extra.Data);
        }
      }
      
      void ParseExtendedPart(Builder& target)
      {
        if (Stream.GetPosition() == sizeof(RawHeader) + sizeof(ExtraRAM)
         && Stream.GetRestSize() >= sizeof(IFFChunkHeader))
        {
          const IFFChunkHeader& hdr = Stream.ReadField<IFFChunkHeader>();
          const std::size_t size = fromLE(hdr.DataSize);
          if (hdr.ID == XID6 && Stream.GetRestSize() >= size)
          {
            const auto chunks = Stream.ReadData(size);
            ParseSubchunks(chunks, target);
          }
          else
          {
            Dbg("Invalid IFFF chunk stored (id=%s, size=%u)", String(hdr.ID.begin(), hdr.ID.end()), size);
            //TODO: fix used size instead
            Stream.ReadRestData();
          }
        }
      }
      
      std::size_t GetUsedData() const
      {
        return Stream.GetPosition();
      }
    private:
      static void ParseID666(const RawHeader::ID666Tag& tag, Builder& target)
      {
        Tag text(tag.TextTag);
        Tag bin(tag.BinTag);
        if (text.GetScore() >= bin.GetScore())
        {
          Dbg("Parse text ID666");
          ParseID666(text, target);
        }
        else
        {
          Dbg("Parse binary ID666");
          ParseID666(bin, target);
        }
      }
      
      static void ParseID666(Tag& tag, Builder& target)
      {
        target.SetTitle(std::move(tag.Song));
        target.SetGame(std::move(tag.Game));
        target.SetDumper(std::move(tag.Dumper));
        target.SetComment(std::move(tag.Comments));
        target.SetDumpDate(std::move(tag.DumpDate));
        target.SetIntro(tag.FadeTime);
        target.SetFade(tag.FadeDuration);
        target.SetArtist(std::move(tag.Artist));
      }
      
      static void ParseSubchunks(Binary::View data, Builder& target)
      {
        try
        {
          for (Binary::DataInputStream stream(data); stream.GetRestSize(); )
          {
            const auto* hdr = stream.PeekField<SubChunkHeader>();
            Require(hdr != nullptr);
            if (hdr->ID == 0 && 0 != (stream.GetPosition() % 4))
            {
              // Despite official format description, subchunks can be not aligned by 4 byte boundary
              stream.Skip(1);
            }
            else
            {
              Dbg("ParseSubchunk id=%u, type=%u, size=%u", uint_t(hdr->ID), uint_t(hdr->Type), fromLE(hdr->DataSize));
              stream.Skip(sizeof(*hdr) + hdr->GetDataSize());
              ParseSubchunk(*hdr, target);
            }
          }
        }
        catch (const std::exception&)
        {
          //ignore
        }
      }
      
      static void ParseSubchunk(const SubChunkHeader& hdr, Builder& target)
      {
          switch (hdr.ID)
          {
          case SubChunkHeader::SongName:
            target.SetTitle(hdr.GetString());
            break;
          case SubChunkHeader::GameName:
            target.SetGame(hdr.GetString());
            break;
          case SubChunkHeader::ArtistName:
            target.SetArtist(hdr.GetString());
            break;
          case SubChunkHeader::DumperName:
            target.SetDumper(hdr.GetString());
            break;
          case SubChunkHeader::Date:
            target.SetDumpDate(DateFromInteger(hdr.GetInteger()));
            break;
          case SubChunkHeader::Comments:
            target.SetComment(hdr.GetString());
            break;
          case SubChunkHeader::IntroductionLength:
            target.SetIntro(hdr.GetTicks());
            break;
          case SubChunkHeader::LoopLength:
            target.SetLoop(hdr.GetTicks());
            break;
          case SubChunkHeader::FadeLength:
            target.SetFade(hdr.GetTicks());
            break;
          default:
            break;
          }
      }
    private:
      Binary::DataInputStream Stream;
    };
    
    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const Binary::View data(rawData);
      if (data.Size() < sizeof(RawHeader))
      {
        return {};
      }
      try
      {
        Format format(data);
        format.ParseMainPart(target);
        format.ParseExtendedPart(target);
        auto subData = rawData.GetSubcontainer(0, format.GetUsedData());
        const std::size_t fixedStart = offsetof(RawHeader, RAM);
        const std::size_t fixedEnd = sizeof(RawHeader);
        return CreateCalculatingCrcContainer(std::move(subData), fixedStart, fixedEnd - fixedStart);
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
  } //namespace SPC

  Decoder::Ptr CreateSPCDecoder()
  {
    return MakePtr<SPC::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
