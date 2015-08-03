/**
* 
* @file
*
* @brief  Abyss' Highest Experience support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "abysshighestexperience.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace AbyssHighestExperience
  {
    typedef boost::array<uint8_t, 4> IdentifierType;
    
    const IdentifierType ID0 = {'T', 'H', 'X', 0};
    const IdentifierType ID1 = {'T', 'H', 'X', 1};
  
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Header
    {
      //+0
      IdentifierType Identifier;
      //+4
      uint16_t NamesOffset;
      //+6
      uint16_t PositionsCountAndFlags;
      //+8
      uint16_t LoopPosition;
      //+10
      uint8_t TrackSize;
      //+11
      uint8_t TracksCount;
      //+12
      uint8_t SamplesCount;
      //+13
      uint8_t SubsongsCount;
    } PACK_POST;
    
    PACK_PRE struct Subsong
    {
      uint16_t Position;
    } PACK_POST;
    
    PACK_PRE struct Position
    {
      PACK_PRE struct Channel
      {
        uint8_t Track;
        int8_t Transposition;
      } PACK_POST;
      boost::array<Channel, 4> Channels;
    } PACK_POST;
    
    PACK_PRE struct Note
    {
      uint16_t NoteSampleCommand;
      uint8_t CommandData;
    } PACK_POST;
    
    PACK_PRE struct Sample
    {
      uint8_t MasterVolume;
      uint8_t Flags;
      uint8_t AttackLength;
      uint8_t AttackVolume;
      uint8_t DecayLength;
      uint8_t DecayVolume;
      uint8_t SustainLength;
      uint8_t ReleaseLength;
      uint8_t ReleaseVolume;
      uint8_t Unused[3];
      uint8_t FilterModulationSpeedLowerLimit;
      uint8_t VibratoDelay;
      uint8_t HardcutVibratoDepth;
      uint8_t VibratoSpeed;
      uint8_t SquareModulationLowerLimit;
      uint8_t SquareModulationUpperLimit;
      uint8_t SquareModulationSpeed;
      uint8_t FilterModulationSpeedUpperLimit;
      uint8_t Speed;
      uint8_t Length;
      
      PACK_PRE struct Entry
      {
        uint32_t Data;
      } PACK_POST;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Header) == 14);
    BOOST_STATIC_ASSERT(sizeof(Subsong) == 2);
    BOOST_STATIC_ASSERT(sizeof(Position) == 8);
    BOOST_STATIC_ASSERT(sizeof(Note) == 3);
    BOOST_STATIC_ASSERT(sizeof(Sample) == 22);
    BOOST_STATIC_ASSERT(sizeof(Sample::Entry) == 4);
    
    const std::size_t MIN_SIZE = sizeof(Header);
    
    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }
    };
    
    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
        , Source(Stream.ReadField<Header>())
        , TracksOffset()
        , NamesOffset()
      {
      }
      
      void Parse(Builder& target)
      {
        ParseSubsongs(target);
        ParsePositions(target);
        TracksOffset = Stream.GetPosition();
        ParseTracks(target);
        ParseSamples(target);
        NamesOffset = Stream.GetPosition();
        ParseCommonProperties(target);
      }
      
      Formats::Chiptune::Container::Ptr GetContainer() const
      {
        const Binary::Container::Ptr rawData = Stream.GetReadData();
        return CreateCalculatingCrcContainer(rawData, TracksOffset, NamesOffset - TracksOffset);
      }
    private:
      void ParseSubsongs(Builder& /*target*/)
      {
        const uint_t count = Source.SubsongsCount;
        Stream.ReadData(count * sizeof(Subsong));
      }
      
      void ParsePositions(Builder& /*target*/)
      {
        const uint_t count = fromBE(Source.PositionsCountAndFlags) & 0xfff;
        Stream.ReadData(count * sizeof(Position));
      }
      
      void ParseTracks(Builder& /*target*/)
      {
        const bool track0Saved = 0 == (fromBE(Source.PositionsCountAndFlags) & 0x8000);
        const uint_t count = Source.TracksCount + track0Saved;
        const uint_t size = Source.TrackSize;
        Stream.ReadData(count * size * sizeof(Note));
      }
      
      void ParseSamples(Builder& /*target*/)
      {
        const uint_t count = Source.SamplesCount;
        for (uint_t smp = 0; smp < count; ++smp)
        {
          const Sample& hdr = Stream.ReadField<Sample>();
          const uint_t size = hdr.Length;
          for (uint_t idx = 0; idx < size; ++idx)
          {
            Stream.ReadData(sizeof(Sample::Entry));
          }
        }
      }
      
      void ParseCommonProperties(Builder& target)
      {
        MetaBuilder& meta = target.GetMetaBuilder();
        const std::string& title = Stream.ReadCString(Stream.GetRestSize());
        meta.SetTitle(FromStdString(title));
        ParseSampleNames();
        if (Source.Identifier == ID0)
        {
          meta.SetProgram(Text::ABYSSHIGHESTEXPERIENCE_EDITOR_OLD);
        }
        else if (Source.Identifier == ID1)
        {
          meta.SetProgram(Text::ABYSSHIGHESTEXPERIENCE_EDITOR_NEW);
        }
      }
      
      void ParseSampleNames()
      {
        const uint_t count = Source.SamplesCount;
        for (uint_t smp = 0; smp < count; ++smp)
        {
          Stream.ReadCString(Stream.GetRestSize());
        }
      }
    private:
      Binary::InputStream Stream;
      const Header& Source;
      std::size_t TracksOffset;
      std::size_t NamesOffset;
    };
    
    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size(rawData.Size());
      if (sizeof(Header) > size)
      {
        return false;
      }
      const Header& header = *static_cast<const Header*>(rawData.Start());
      if (header.Identifier != ID0
       && header.Identifier != ID1)
      {
        return false;
      }
      const std::size_t subsongsSize = sizeof(Subsong) * header.SubsongsCount;
      const std::size_t positionsSize = sizeof(Position) * (fromBE(header.PositionsCountAndFlags) & 0xfff);
      const std::size_t tracksSize = sizeof(Note) * header.TrackSize * header.TracksCount;
      const std::size_t minSamplesSize = sizeof(Sample) * header.SamplesCount;
      return size > sizeof(header) + subsongsSize + positionsSize + tracksSize + minSamplesSize;
    }
    
    const std::string FORMAT(
      "'T'H'X 00-01" //signature
      "??"           //names offset
      "%xxxx00xx ?"  //flags and positions count 0-0x3e7
      "%000000xx ?"  //restart position 0-0x3e6
      "01-40"        //track len
      "?"            //tracks count
      "00-3f"        //samples count
      "?"            //subsongs count
    );
    
    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::ABYSSHIGHESTEXPERIENCE_DECODER_DESCRIPTION;
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

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        Format format(data);
        format.Parse(target);
        return format.GetContainer();
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
  }//namespace AbyssHighestExperience
  
  Decoder::Ptr CreateAbyssHighestExperienceDecoder()
  {
    return boost::make_shared<AbyssHighestExperience::Decoder>();
  }
}
}
