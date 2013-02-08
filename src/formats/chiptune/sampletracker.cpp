/*
Abstract:
  SampleTracker format description implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "digital_detail.h"
#include "sampletracker.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <range_checker.h>
//library includes
#include <binary/container_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//std includes
#include <cstring>
#include <set>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/mem_fn.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::SampleTracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace SampleTracker
  {
    const std::size_t MAX_MODULE_SIZE = 0x87a0;
    const std::size_t MAX_POSITIONS_COUNT = 0x40;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 16;
    const std::size_t CHANNELS_COUNT = 3;
    const std::size_t SAMPLES_COUNT = 16;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Pattern
    {
      static const uint8_t LIMIT = 0xff;

      PACK_PRE struct Line
      {
        uint8_t Note[CHANNELS_COUNT];
        uint8_t Sample[CHANNELS_COUNT];
      } PACK_POST;

      Line Lines[MAX_PATTERN_SIZE];
      uint8_t Limit;
    } PACK_POST;

    PACK_PRE struct SampleInfo
    {
      uint8_t AddrHi;
      uint8_t SizeHiDoubled;
    } PACK_POST;

    PACK_PRE struct Header
    {
      //+0
      uint8_t Tempo;
      //+1
      boost::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
      //+0x41
      boost::array<uint16_t, MAX_POSITIONS_COUNT> PositionsPtrs;
      //+0xc1
      char Title[10];
      //+0xcb
      uint8_t LastPositionDoubled;
      //+0xcc
      boost::array<Pattern, PATTERNS_COUNT> Patterns;
      //+0x18dc
      SampleInfo SampleDescriptions[SAMPLES_COUNT];
      //+0x18fc
      uint8_t Padding[4];
      //+0x1900
      boost::array<uint8_t, 10> SampleNames[SAMPLES_COUNT];
      //+0x19a0
      uint8_t Samples[1];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Header) == 0x19a1);

    const uint_t NOTE_EMPTY = 0;
    const uint_t NOTE_BASE = 1;

    const uint_t SAMPLE_EMPTY = 0;
    const uint_t PAUSE = 0x10;
    const uint_t SAMPLE_BASE = 0x25;

    const uint_t MODULE_BASE = 0x7260;
    const uint_t SAMPLES_ADDR = MODULE_BASE + offsetof(Header, Samples);
    const uint_t SAMPLES_LIMIT_ADDR = 0xfa00;

    const std::size_t MIN_SIZE = sizeof(Header) + 256;

    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*safe_ptr_cast<const Header*>(RawData.Start()))
        , Ranges(RangeChecker::Create(rawData.Size()))
        , FixedRanges(RangeChecker::Create(rawData.Size()))
      {
        //info
        AddRange(0, offsetof(Header, Samples));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        target.SetTitle(FromCharArray(Source.Title));
        target.SetProgram(Text::SAMPLETRACKER_DECODER_DESCRIPTION);
      }

      void ParsePositions(Builder& target) const
      {
        const uint_t positionsCount = Source.LastPositionDoubled / 2;
        Require(Math::InRange<uint_t>(positionsCount + 1, 1, MAX_POSITIONS_COUNT));

        std::vector<uint_t> positions(positionsCount);
        std::transform(Source.Positions.begin(), Source.Positions.begin() + positionsCount, positions.begin(),
          std::bind2nd(std::minus<uint8_t>(), uint8_t(1)));
        target.SetPositions(positions, 0);
        Dbg("Positions: %1%", positions.size());
      }

      void ParsePatterns(const Digital::Indices& pats, Builder& target) const
      {
        Dbg("Parse %1% patterns", pats.size());
        Require(!pats.empty());
        for (Digital::Indices::const_iterator it = pats.begin(), lim = pats.end(); it != lim; ++it)
        {
          const uint_t patIndex = *it;
          Require(Math::InRange<uint_t>(patIndex + 1, 1, PATTERNS_COUNT));
          Dbg("Parse pattern %1%", patIndex);
          target.StartPattern(patIndex);
          ParsePattern(Source.Patterns[patIndex], target);
          AddFixedRange(offsetof(Header, Patterns) + patIndex * sizeof(Pattern), sizeof(Pattern));
        }
      }

      void ParseSamples(const Digital::Indices& sams, Builder& target) const
      {
        Dbg("Parse %1% samples", sams.size());
        Require(!sams.empty());
        std::size_t validSamples = 0;
        for (Digital::Indices::const_iterator it = sams.begin(), lim = sams.end(); it != lim; ++it)
        {
          const uint_t samIdx = *it;
          Require(Math::InRange<uint_t>(samIdx + 1, 1, SAMPLES_COUNT));
          const SampleInfo& info = Source.SampleDescriptions[samIdx];
          const std::size_t absAddr = 256 * info.AddrHi;
          const std::size_t maxSize = 128 * info.SizeHiDoubled;
          if (absAddr && absAddr + maxSize <= SAMPLES_LIMIT_ADDR)
          {
            Dbg("Sample %1%: start=#%2$04x size=#%3$04x", 
              samIdx, absAddr, maxSize);
            const uint8_t* const sampleStart = Source.Samples + (absAddr - SAMPLES_ADDR);
            const uint8_t* const sampleEnd = std::find(sampleStart, sampleStart + maxSize, 0);
            const std::size_t sampleOffset = absAddr - MODULE_BASE;
            const std::size_t sampleSize = sampleEnd - sampleStart;
            AddRange(sampleOffset, sampleSize);
            if (const Binary::Data::Ptr content = RawData.GetSubcontainer(sampleOffset, sampleSize))
            {
              target.SetSample(samIdx, sampleSize, content, false);
              ++validSamples;
              continue;
            }
          }
          Dbg(" Stub sample %1%", samIdx);
          const uint8_t dummy = 128;
          target.SetSample(samIdx, 0, Binary::CreateContainer(&dummy, sizeof(dummy)), false);
        }
        Require(validSamples != 0);
      }

      std::size_t GetSize() const
      {
        return Ranges->GetAffectedRange().second;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return FixedRanges->GetAffectedRange();
      }
    private:
      static void ParsePattern(const Pattern& src, Builder& target)
      {
        const uint_t lastLine = MAX_PATTERN_SIZE - 1;
        uint_t lineNum = 0;
        for (; lineNum <= lastLine ; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          if (srcLine.Note[0] == Pattern::LIMIT)
          {
            break;
          }

          if (lineNum != lastLine && IsEmptyLine(srcLine))
          {
            continue;
          }
          target.StartLine(lineNum);
          ParseLine(srcLine, target);
        }
      }

      static void ParseLine(const Pattern::Line& srcLine, Builder& target)
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          target.StartChannel(chanNum);
          const uint_t note = srcLine.Note[chanNum];
          if (NOTE_EMPTY != note)
          {
            target.SetNote(note - NOTE_BASE);
          }
          switch (const uint_t sample = srcLine.Sample[chanNum])
          {
          case PAUSE:
            target.SetRest();
            break;
          case SAMPLE_EMPTY:
            break;
          default:
            if (sample < SAMPLES_COUNT + SAMPLE_BASE)
            {
              target.SetSample(sample - SAMPLE_BASE);
            }
            break;
          }
        }
      }

      static bool IsEmptyLine(const Pattern::Line& srcLine)
      {
        return IsEmptyChannel(0, srcLine)
            && IsEmptyChannel(1, srcLine)
            && IsEmptyChannel(2, srcLine)
        ;
      }

      static bool IsEmptyChannel(uint_t chan, const Pattern::Line& srcLine)
      {
        return srcLine.Note[chan] == NOTE_EMPTY && (srcLine.Sample[chan] == SAMPLE_EMPTY || srcLine.Sample[chan] >= SAMPLE_BASE + SAMPLES_COUNT);
      }

      void AddRange(std::size_t start, std::size_t size) const
      {
        Require(Ranges->AddRange(start, size));
      }

      void AddFixedRange(std::size_t start, std::size_t size) const
      {
        Require(FixedRanges->AddRange(start, size));
      }
    private:
      const Binary::Container& RawData;
      const Header& Source;
      const RangeChecker::Ptr Ranges;
      const RangeChecker::Ptr FixedRanges;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size = rawData.Size();
      if (size < sizeof(Header))
      {
        return false;
      }
      const Header& header = *safe_ptr_cast<const Header*>(rawData.Start());
      return header.LastPositionDoubled >= 2
          && header.LastPositionDoubled <= 128
          && 0 == (header.LastPositionDoubled & 1)
      ;
    }

    const std::string FORMAT(
      "01-10" //tempo
      "01-10{64}" //positions
      "?73-8b"  //first position ptr
      "?{126}"  //other ptrs
      "20-7f{10}" //title
      "%xxxxxxx0" //doubled last position
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SAMPLETRACKER_DECODER_DESCRIPTION;
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
        Builder& stub = Digital::GetStubBuilder();
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

        Digital::StatisticCollectionBuilder statistic(target);
        format.ParsePositions(statistic);
        const Digital::Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Digital::Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);

        const Binary::Container::Ptr subData = data.GetSubcontainer(0, format.GetSize());
        const RangeChecker::Range fixedRange = format.GetFixedArea();
        return CreateCalculatingCrcContainer(subData, fixedRange.first, fixedRange.second - fixedRange.first);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }
  }//namespace SampleTracker

  Decoder::Ptr CreateSampleTrackerDecoder()
  {
    return boost::make_shared<SampleTracker::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
