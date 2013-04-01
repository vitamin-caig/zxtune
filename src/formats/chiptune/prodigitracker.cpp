/*
Abstract:
  ProDigiTracker format description implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "prodigitracker.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <range_checker.h>
//library includes
#include <binary/container_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::ProDigiTracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace ProDigiTracker
  {
    const uint_t ORNAMENTS_COUNT = 11;
    const uint_t SAMPLES_COUNT = 16;
    const uint_t POSITIONS_COUNT = 240;
    const uint_t PAGES_COUNT = 5;
    const uint_t CHANNELS_COUNT = 4;
    const uint_t PATTERN_SIZE = 64;
    const uint_t PATTERNS_COUNT = 32;
    const std::size_t ZX_PAGE_SIZE = 0x4000;
    const std::size_t PAGES_START = 0xc000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    typedef boost::array<int8_t, 16> Ornament;

    PACK_PRE struct OrnamentLoop
    {
      uint8_t Begin;
      uint8_t End;
    } PACK_POST;

    PACK_PRE struct Sample
    {
      uint8_t Name[8];
      uint16_t Start;
      uint16_t Size;
      uint16_t Loop;
      uint8_t Page;
      uint8_t Padding;
    } PACK_POST;

    const uint_t NOTE_EMPTY = 0;

    enum
    {
      CMD_SPECIAL = 0, //see parameter
      CMD_SPEED = 1,   //parameter- speed
      CMD_1 = 2,       //????
      CMD_2 = 3,       //????

      COMMAND_NOORNAMENT = 15,
      COMMAND_BLOCKCHANNEL = 14,
      COMMAND_ENDPATTERN = 13,
      COMMAND_CONTSAMPLE = 12,
      COMMAND_NONE = 0
      //else ornament + 1
    };

    PACK_PRE struct Note
    {
      //ccnnnnnn
      //sssspppp
      //c- command
      //n- note
      //p- parameter
      //s- sample
      uint_t GetNote() const
      {
        return NoteComm & 63;
      }

      uint_t GetCommand() const
      {
        return (NoteComm & 192) >> 6;
      }

      uint_t GetParameter() const
      {
        return ParamSample & 15;
      }

      uint_t GetSample() const
      {
        return (ParamSample & 240) >> 4;
      }

      bool IsEnd() const
      {
        return GetCommand() == CMD_SPECIAL && GetParameter() == COMMAND_ENDPATTERN;
      }

      uint8_t NoteComm;
      uint8_t ParamSample;
    } PACK_POST;

    typedef boost::array<Note, CHANNELS_COUNT> Line;

    typedef boost::array<Line, PATTERN_SIZE> Pattern;

    PACK_PRE struct Header
    {
      boost::array<Ornament, ORNAMENTS_COUNT> Ornaments;
      boost::array<OrnamentLoop, ORNAMENTS_COUNT> OrnLoops;
      uint8_t Padding1[6];
      char Title[32];
      uint8_t Tempo;
      uint8_t Start;
      uint8_t Loop;
      uint8_t Length;
      uint8_t Padding2[16];
      boost::array<Sample, SAMPLES_COUNT> Samples;
      boost::array<uint8_t, POSITIONS_COUNT> Positions;
      uint16_t LastDatas[PAGES_COUNT];
      uint8_t FreeRAM;
      uint8_t Padding3[5];
      boost::array<Pattern, PATTERNS_COUNT> Patterns;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Ornament) == 16);
    BOOST_STATIC_ASSERT(sizeof(OrnamentLoop) == 2);
    BOOST_STATIC_ASSERT(sizeof(Sample) == 16);
    BOOST_STATIC_ASSERT(sizeof(Note) == 2);
    BOOST_STATIC_ASSERT(sizeof(Pattern) == 512);
    BOOST_STATIC_ASSERT(sizeof(Header) == 0x4300);

    const std::size_t MODULE_SIZE = sizeof(Header) + PAGES_COUNT * ZX_PAGE_SIZE;

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::Data::Ptr /*content*/) {}
      virtual void SetOrnament(uint_t /*index*/, std::size_t /*loop*/, const std::vector<int_t>& /*ornament*/) {}
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}
      virtual void StartPattern(uint_t /*index*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void SetTempo(uint_t /*tempo*/) {}
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
    };

    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, PATTERNS_COUNT - 1)
        , UsedSamples(0, SAMPLES_COUNT - 1)
        , UsedOrnaments(0, ORNAMENTS_COUNT - 1)
      {
      }

      virtual MetaBuilder& GetMetaBuilder()
      {
        return Delegate.GetMetaBuilder();
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        return Delegate.SetInitialTempo(tempo);
      }

      virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr data)
      {
        return Delegate.SetSample(index, loop, data);
      }

      virtual void SetOrnament(uint_t index, std::size_t loop, const std::vector<int_t>& ornament)
      {
        return Delegate.SetOrnament(index, loop, ornament);
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        UsedPatterns.Assign(positions.begin(), positions.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(positions, loop);
      }

      virtual void StartPattern(uint_t index)
      {
        return Delegate.StartPattern(index);
      }

      virtual void StartLine(uint_t index)
      {
        return Delegate.StartLine(index);
      }

      virtual void SetTempo(uint_t tempo)
      {
        return Delegate.SetTempo(tempo);
      }

      virtual void StartChannel(uint_t index)
      {
        return Delegate.StartChannel(index);
      }

      virtual void SetRest()
      {
        return Delegate.SetRest();
      }

      virtual void SetNote(uint_t note)
      {
        return Delegate.SetNote(note);
      }

      virtual void SetSample(uint_t sample)
      {
        UsedSamples.Insert(sample);
        return Delegate.SetSample(sample);
      }

      virtual void SetOrnament(uint_t ornament)
      {
        UsedOrnaments.Insert(ornament);
        return Delegate.SetOrnament(ornament);
      }

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

      const Indices& GetUsedSamples() const
      {
        Require(!UsedSamples.Empty());
        return UsedSamples;
      }

      const Indices& GetUsedOrnaments() const
      {
        return UsedOrnaments;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
      Indices UsedOrnaments;
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*safe_ptr_cast<const Header*>(RawData.Start()))
        , FixedRanges(RangeChecker::Create(RawData.Size()))
      {
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetTitle(FromCharArray(Source.Title));
        meta.SetProgram(Text::PRODIGITRACKER_DECODER_DESCRIPTION);
      }

      void ParsePositions(Builder& target) const
      {
        const std::vector<uint_t> positions(Source.Positions.begin(), Source.Positions.begin() + Source.Length);
        target.SetPositions(positions, Source.Loop);
        Dbg("Positions: %1%, loop to %2%", positions.size(), unsigned(Source.Loop));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          target.StartPattern(patIndex);
          ParsePattern(patIndex, target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        const uint8_t* const moduleStart = safe_ptr_cast<const uint8_t*>(&Source);
        const uint8_t* const samplesStart = safe_ptr_cast<const uint8_t*>(&Source + 1);
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const Sample& descr = Source.Samples[samIdx];
          const std::size_t start = fromLE(descr.Start);
          const std::size_t loop = fromLE(descr.Loop);
          std::size_t size = fromLE(descr.Size);
          if (descr.Page < PAGES_COUNT && start >= PAGES_START && size != 0)
          {
            Dbg("Sample %1%: start=#%2$04x loop=#%3$04x size=#%5$04x",
              samIdx, start, loop, size);
            const uint8_t* const sampleData = samplesStart + ZX_PAGE_SIZE * GetPageOrder(descr.Page) + (start - PAGES_START);
            while (--size && sampleData[size] == 0) {};
            ++size;
            if (const Binary::Data::Ptr content = RawData.GetSubcontainer(sampleData - moduleStart, size))
            {
              target.SetSample(samIdx, loop >= start ? loop - start : size, content);
              continue;
            }
          }
          Dbg(" Stub sample %1%", samIdx);
          const uint8_t dummy = 128;
          target.SetSample(samIdx, 0, Binary::CreateContainer(&dummy, sizeof(dummy)));
        }
      }

      void ParseOrnaments(const Indices& orns, Builder& target) const
      {
        for (Indices::Iterator it = orns.Items(); it; ++it)
        {
          if (const uint_t ornIdx = *it)
          {
            const Ornament& orn = Source.Ornaments[ornIdx - 1];
            const OrnamentLoop& loop = Source.OrnLoops[ornIdx - 1];
            std::vector<int_t> data(loop.End);
            std::transform(orn.begin(), orn.begin() + loop.End, data.begin(), std::bind2nd(std::divides<int8_t>(), 2));
            target.SetOrnament(ornIdx, loop.Begin, data);
          }
          else
          {
            target.SetOrnament(ornIdx, 0, std::vector<int_t>());
          }
        }
      }

      RangeChecker::Range GetFixedArea() const
      {
        return FixedRanges->GetAffectedRange();
      }
    private:
      static uint_t GetPageOrder(uint_t page)
      {
        //1,3,4,6,7
        switch (page)
        {
        case 1:
          return 0;
        case 3:
          return 1;
        case 4:
          return 2;
        case 6:
          return 3;
        case 7:
          return 4;
        default:
          assert(!"Invalid page");
          return 0;
        }
      }

      void ParsePattern(uint_t idx, Builder& target) const
      {
        const Pattern& src = Source.Patterns[idx];
        uint_t lineNum = 0;
        for (; lineNum < PATTERN_SIZE; ++lineNum)
        {
          const Line& srcLine = src[lineNum];
          if (IsLastLine(srcLine))
          {
            break;
          }
          target.StartLine(lineNum);
          ParseLine(srcLine, target);
        }
        const std::size_t patStart = offsetof(Header, Patterns) + idx * sizeof(Pattern);
        const std::size_t patSize = lineNum * sizeof(Line);
        AddFixedRange(patStart, patSize);
      }

      static bool IsLastLine(const Line& line)
      {
        return line[0].IsEnd() || line[1].IsEnd() || line[2].IsEnd() || line[3].IsEnd();
      }

      static void ParseLine(const Line& line, Builder& target)
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          target.StartChannel(chanNum);
          const Note& note = line[chanNum];
          const uint_t halftones = note.GetNote();
          int_t sample = -1;
          if (halftones != NOTE_EMPTY)
          {
            target.SetNote(halftones - 1);
            sample = note.GetSample();
          }

          switch (note.GetCommand())
          {
          case CMD_SPEED:
            target.SetTempo(note.GetParameter());
            break;
          case CMD_SPECIAL:
            {
              switch (uint_t param = note.GetParameter())
              {
              case COMMAND_NONE:
                if (halftones == NOTE_EMPTY)
                {
                  break;
                }
              case COMMAND_NOORNAMENT:
                target.SetOrnament(0);
                break;
              case COMMAND_CONTSAMPLE:
                sample = -1;
                break;
              case COMMAND_ENDPATTERN:
                break;
              case COMMAND_BLOCKCHANNEL:
                target.SetRest();
                break;
              default:
                target.SetOrnament(param);
                break;
              }
            }
          }

          if (sample != -1)
          {
            target.SetSample(sample);
          }
        }
      }

      void AddFixedRange(std::size_t start, std::size_t size) const
      {
        Require(FixedRanges->AddRange(start, size));
      }
    private:
      const Binary::Container& RawData;
      const Header& Source;
      const RangeChecker::Ptr FixedRanges;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size(rawData.Size());
      if (sizeof(Header) > size)
      {
        return false;
      }
      const Header& header = *safe_ptr_cast<const Header*>(rawData.Start());
      if (header.Loop > header.Length)
      {
        return false;
      }
      return true;
    }

    const std::string FORMAT(
      //boost::array<PDTOrnament, ORNAMENTS_COUNT> Ornaments;
      "(%xxxxxxx0{16}){11}"
      //boost::array<PDTOrnamentLoop, ORNAMENTS_COUNT> OrnLoops;
      "?{22}"
      //uint8_t Padding1[6];
      "?{6}"
      //char Title[32];
      "?{32}"
      //uint8_t Tempo;
      "03-63"
      //uint8_t Start;
      "00-ef"
      //uint8_t Loop;
      "00-ef"
      //uint8_t Length;
      "01-f0"
      //uint8_t Padding2[16];
      "00{16}"
      //boost::array<PDTSample, SAMPLES_COUNT> Samples;
      /*
      uint8_t Name[8];
      uint16_t Start;
      uint16_t Size;
      uint16_t Loop;
      uint8_t Page;
      uint8_t Padding;
      */
      "(???????? ?5x|3x|c0-ff ?00-40 ?5x|3x|c0-ff 00|01|03|04|06|07 00){16}"
      //boost::array<uint8_t, POSITIONS_COUNT> Positions;
      "(00-1f){240}"
      //uint16_t LastDatas[PAGES_COUNT];
      "(?c0-ff){5}"
      /*
      uint8_t FreeRAM;
      uint8_t Padding3[5];
      */
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT, MODULE_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::PRODIGITRACKER_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return FastCheck(rawData) && Format->Match(rawData);
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
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectionBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

        const Binary::Container::Ptr subData = data.GetSubcontainer(0, MODULE_SIZE);
        const RangeChecker::Range fixedRange = format.GetFixedArea();
        return CreateCalculatingCrcContainer(subData, fixedRange.first, fixedRange.second - fixedRange.first);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace ProDigiTracker

  Decoder::Ptr CreateProDigiTrackerDecoder()
  {
    return boost::make_shared<ProDigiTracker::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
