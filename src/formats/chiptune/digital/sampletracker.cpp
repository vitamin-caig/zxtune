/**
 *
 * @file
 *
 * @brief  SampleTracker support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/digital/sampletracker.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/digital/digital_detail.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <range_checker.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/optimize.h>
// std includes
#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace SampleTracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::SampleTracker");

    const Char DESCRIPTION[] = "Sample Tracker v2.x";

    // const std::size_t MAX_MODULE_SIZE = 0x87a0;
    const std::size_t MAX_POSITIONS_COUNT = 0x40;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 16;
    const std::size_t CHANNELS_COUNT = 3;
    const std::size_t SAMPLES_COUNT = 16;

    struct Pattern
    {
      static const uint8_t LIMIT = 0xff;

      struct Line
      {
        uint8_t Note[CHANNELS_COUNT];
        uint8_t Sample[CHANNELS_COUNT];
      };

      Line Lines[MAX_PATTERN_SIZE];
      uint8_t Limit;
    };

    struct SampleInfo
    {
      uint8_t AddrHi;
      uint8_t SizeHiDoubled;
    };

    struct Header
    {
      //+0
      uint8_t Tempo;
      //+1
      std::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
      //+0x41
      std::array<le_uint16_t, MAX_POSITIONS_COUNT> PositionsPtrs;
      //+0xc1
      std::array<char, 10> Title;
      //+0xcb
      uint8_t LastPositionDoubled;
      //+0xcc
      std::array<Pattern, PATTERNS_COUNT> Patterns;
      //+0x18dc
      SampleInfo SampleDescriptions[SAMPLES_COUNT];
      //+0x18fc
      uint8_t Padding[4];
      //+0x1900
      std::array<std::array<char, 10>, SAMPLES_COUNT> SampleNames;
      //+0x19a0
      uint8_t Samples[1];
    };

    static_assert(sizeof(Header) * alignof(Header) == 0x19a1, "Invalid layout");

    const uint_t NOTE_EMPTY = 0;
    const uint_t NOTE_BASE = 1;

    const uint_t SAMPLE_EMPTY = 0;
    const uint_t PAUSE = 0x10;
    const uint_t SAMPLE_BASE = 0x25;

    const uint_t MODULE_BASE = 0x7260;
    const uint_t SAMPLES_ADDR = MODULE_BASE + offsetof(Header, Samples);
    const uint_t SAMPLES_LIMIT_ADDR = 0xfa00;

    const std::size_t MIN_SIZE = sizeof(Header) + 256;

    const uint64_t Z80_FREQ = 3500000;
    // 109+113+113+10=345 ticks/out cycle = 10144 outs/sec
    const uint_t TICKS_PER_CYCLE = 109 + 113 + 113 + 10;
    // C-1 step 22/256 32.7Hz = ~871 samples/sec
    const uint_t C_1_STEP = 22;
    const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;

    class Format
    {
    public:
      explicit Format(Binary::View rawData)
        : RawData(rawData)
        , Source(*RawData.As<Header>())
        , Ranges(RangeChecker::Create(rawData.Size()))
        , FixedRanges(RangeChecker::Create(rawData.Size()))
      {
        // info
        AddRange(0, offsetof(Header, Samples));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetTitle(Strings::OptimizeAscii(Source.Title));
        meta.SetProgram(DESCRIPTION);
        Strings::Array names;
        names.reserve(Source.SampleNames.size());
        for (const auto& name : Source.SampleNames)
        {
          names.push_back(Strings::OptimizeAscii(name));
        }
        meta.SetStrings(names);
      }

      void ParsePositions(Builder& target) const
      {
        const uint_t positionsCount = Source.LastPositionDoubled / 2;
        Require(Math::InRange<uint_t>(positionsCount + 1, 1, MAX_POSITIONS_COUNT));

        Digital::Positions positions;
        positions.Lines.resize(positionsCount);
        std::transform(Source.Positions.begin(), Source.Positions.begin() + positionsCount, positions.Lines.begin(),
                       [](auto b) { return b - 1; });
        Dbg("Positions: {}", positions.GetSize());
        target.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        Dbg("Parse {} patterns", pats.Count());
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          PatternBuilder& patBuilder = target.StartPattern(patIndex);
          ParsePattern(Source.Patterns[patIndex], patBuilder, target);
          AddFixedRange(offsetof(Header, Patterns) + patIndex * sizeof(Pattern), sizeof(Pattern));
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        Dbg("Parse {} samples", sams.Count());
        target.SetSamplesFrequency(SAMPLES_FREQ);
        std::size_t validSamples = 0;
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          if (const auto content = GetSample(samIdx))
          {
            target.SetSample(samIdx, content.Size(), content, false);
            ++validSamples;
          }
          else
          {
            Dbg(" Stub sample {}", samIdx);
            const uint8_t dummy[] = {128};
            target.SetSample(samIdx, 0, dummy, false);
          }
        }
        if (sams.Maximum() != SAMPLES_COUNT - 1)
        {
          GetSample(SAMPLES_COUNT - 1);
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
      static void ParsePattern(const Pattern& src, PatternBuilder& patBuilder, Builder& target)
      {
        const uint_t lastLine = MAX_PATTERN_SIZE - 1;
        for (uint_t lineNum = 0; lineNum <= lastLine; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          if (srcLine.Note[0] == Pattern::LIMIT)
          {
            patBuilder.Finish(lineNum);
            break;
          }

          if (lineNum != lastLine && IsEmptyLine(srcLine))
          {
            continue;
          }
          patBuilder.StartLine(lineNum);
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
        return IsEmptyChannel(0, srcLine) && IsEmptyChannel(1, srcLine) && IsEmptyChannel(2, srcLine);
      }

      static bool IsEmptyChannel(uint_t chan, const Pattern::Line& srcLine)
      {
        return srcLine.Note[chan] == NOTE_EMPTY
               && (srcLine.Sample[chan] == SAMPLE_EMPTY || srcLine.Sample[chan] >= SAMPLE_BASE + SAMPLES_COUNT);
      }

      Binary::View GetSample(uint_t samIdx) const
      {
        const SampleInfo& info = Source.SampleDescriptions[samIdx];
        const std::size_t absAddr = 256 * info.AddrHi;
        const std::size_t maxSize = 128 * info.SizeHiDoubled;
        if (absAddr < SAMPLES_ADDR || absAddr + maxSize > SAMPLES_LIMIT_ADDR)
        {
          return {nullptr, 0};
        }
        const std::size_t sampleOffset = offsetof(Header, Samples) + (absAddr - SAMPLES_ADDR);
        if (sampleOffset >= RawData.Size())
        {
          return {nullptr, 0};
        }
        const std::size_t sampleAvail = std::min(maxSize, RawData.Size() - sampleOffset);
        Dbg("Sample {}: start=#{:04x} size=#{:04x} (avail=#{:04x})", samIdx, absAddr, maxSize, sampleAvail);
        const uint8_t* const sampleStart = Source.Samples + (absAddr - SAMPLES_ADDR);
        const uint8_t* const sampleEnd = std::find(sampleStart, sampleStart + sampleAvail, 0);
        const std::size_t sampleSize = sampleEnd - sampleStart;
        AddRange(sampleOffset, sampleAvail);
        return RawData.SubView(sampleOffset, sampleSize);
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
      const Binary::View RawData;
      const Header& Source;
      const RangeChecker::Ptr Ranges;
      const RangeChecker::Ptr FixedRanges;
    };

    bool FastCheck(Binary::View rawData)
    {
      const auto* header = rawData.As<Header>();
      return header && header->LastPositionDoubled >= 2 && header->LastPositionDoubled <= 128
             && 0 == (header->LastPositionDoubled & 1);
    }

    const auto FORMAT =
        "01-10"      // tempo
        "01-10{64}"  // positions
        "?73-8b"     // first position ptr
        "?{126}"     // other ptrs
        "20-7f{10}"  // title
        "%xxxxxxx0"  // doubled last position
        ""_sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
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
        return FastCheck(rawData) && Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return {};
        }
        Builder& stub = Digital::GetStubBuilder();
        return Parse(rawData, stub);
      }

    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const Binary::View data(rawData);
      if (!FastCheck(data))
      {
        return {};
      }

      try
      {
        const Format format(data);

        format.ParseCommonProperties(target);

        Digital::StatisticCollectionBuilder statistic(target, PATTERNS_COUNT, SAMPLES_COUNT);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);

        Require(format.GetSize() >= MIN_SIZE);
        auto subData = rawData.GetSubcontainer(0, format.GetSize());
        const auto fixedRange = format.GetFixedArea();
        return CreateCalculatingCrcContainer(std::move(subData), fixedRange.first,
                                             fixedRange.second - fixedRange.first);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return {};
      }
    }
  }  // namespace SampleTracker

  Decoder::Ptr CreateSampleTrackerDecoder()
  {
    return MakePtr<SampleTracker::Decoder>();
  }
}  // namespace Formats::Chiptune
