/**
 *
 * @file
 *
 * @brief  SQDigitalTracker support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/digital/sqdigitaltracker.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/format.h>
#include <strings/optimize.h>
#include <tools/indices.h>
#include <tools/range_checker.h>
// std includes
#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace SQDigitalTracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::SQDigitalTracker");

    const auto DESCRIPTION = "SQ Digital Tracker"sv;

    // const std::size_t MAX_MODULE_SIZE = 0x4400 + 8 * 0x4000;
    const std::size_t MAX_POSITIONS_COUNT = 100;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 32;
    const std::size_t CHANNELS_COUNT = 4;
    const std::size_t SAMPLES_COUNT = 16;

    const std::size_t BIG_SAMPLE_ADDR = 0x8000;
    const std::size_t SAMPLES_ADDR = 0xc000;
    const std::size_t SAMPLES_LIMIT = 0x10000;

    struct Pattern
    {
      struct Line
      {
        struct Channel
        {
          uint8_t NoteCmd;
          uint8_t SampleEffect;

          bool IsEmpty() const
          {
            return 0 == NoteCmd;
          }

          bool IsRest() const
          {
            return 61 == NoteCmd;
          }

          const uint8_t* GetNewTempo() const
          {
            return 62 == NoteCmd ? &SampleEffect : nullptr;
          }

          bool IsEndOfPattern() const
          {
            return 63 == NoteCmd;
          }

          const uint8_t* GetVolumeSlidePeriod() const
          {
            return 64 == NoteCmd ? &SampleEffect : nullptr;
          }

          int_t GetVolumeSlideDirection() const
          {
            // 0 - no slide,
            // 1 - -1
            // 2,3 - +1
            if (const int_t val = NoteCmd >> 6)
            {
              return val == 1 ? -1 : +1;
            }
            else
            {
              return val;
            }
          }

          uint_t GetNote() const
          {
            return (NoteCmd & 63) - 1;
          }

          uint_t GetSample() const
          {
            return SampleEffect & 15;
          }

          uint_t GetSampleVolume() const
          {
            return SampleEffect >> 4;
          }
        };

        Channel Channels[CHANNELS_COUNT];

        bool IsEmpty() const
        {
          return Channels[0].IsEmpty() && Channels[1].IsEmpty() && Channels[2].IsEmpty() && Channels[3].IsEmpty();
        }
      };

      Line Lines[MAX_PATTERN_SIZE];
    };

    struct SampleInfo
    {
      le_uint16_t Start;
      le_uint16_t Loop;
      uint8_t IsLooped;
      uint8_t Bank;
      uint8_t Padding[2];
    };

    struct LayoutInfo
    {
      le_uint16_t Address;
      uint8_t Bank;
      uint8_t Sectors;
    };

    struct Header
    {
      //+0
      uint8_t SamplesData[0x80];
      //+0x80
      uint8_t Banks[0x40];
      //+0xc0
      std::array<LayoutInfo, 8> Layouts;
      //+0xe0
      uint8_t Padding1[0x20];
      //+0x100
      std::array<char, 0x20> Title;
      //+0x120
      SampleInfo Samples[SAMPLES_COUNT];
      //+0x1a0
      std::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
      //+0x204
      uint8_t PositionsLimit;
      //+0x205
      uint8_t Padding2[0xb];
      //+0x210
      uint8_t Tempo;
      //+0x211
      uint8_t Loop;
      //+0x212
      uint8_t LastPosition;
      //+0x213
      uint8_t Padding3[0xed];
      //+0x300
      std::array<std::array<char, 8>, SAMPLES_COUNT> SampleNames;
      //+0x380
      uint8_t Padding4[0x80];
      //+0x400
      std::array<Pattern, PATTERNS_COUNT> Patterns;
      //+0x4400
    };

    static_assert(sizeof(Header) * alignof(Header) == 0x4400, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header);

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetInitialTempo(uint_t /*tempo*/) override {}
      void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::View /*content*/) override {}
      void SetPositions(Positions /*positions*/) override {}

      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }

      void StartChannel(uint_t /*index*/) override {}
      void SetRest() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetSample(uint_t /*sample*/) override {}
      void SetVolume(uint_t /*volume*/) override {}
      void SetVolumeSlidePeriod(uint_t /*period*/) override {}
      void SetVolumeSlideDirection(int_t /*direction*/) override {}
    };

    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, PATTERNS_COUNT - 1)
        , UsedSamples(0, SAMPLES_COUNT - 1)
      {}

      MetaBuilder& GetMetaBuilder() override
      {
        return Delegate.GetMetaBuilder();
      }

      void SetInitialTempo(uint_t tempo) override
      {
        return Delegate.SetInitialTempo(tempo);
      }

      void SetSample(uint_t index, std::size_t loop, Binary::View data) override
      {
        return Delegate.SetSample(index, loop, data);
      }

      void SetPositions(Positions positions) override
      {
        UsedPatterns.Assign(positions.Lines.begin(), positions.Lines.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(std::move(positions));
      }

      PatternBuilder& StartPattern(uint_t index) override
      {
        return Delegate.StartPattern(index);
      }

      void StartChannel(uint_t index) override
      {
        return Delegate.StartChannel(index);
      }

      void SetRest() override
      {
        return Delegate.SetRest();
      }

      void SetNote(uint_t note) override
      {
        return Delegate.SetNote(note);
      }

      void SetSample(uint_t sample) override
      {
        UsedSamples.Insert(sample);
        return Delegate.SetSample(sample);
      }

      void SetVolume(uint_t volume) override
      {
        return Delegate.SetVolume(volume);
      }

      void SetVolumeSlidePeriod(uint_t period) override
      {
        return Delegate.SetVolumeSlidePeriod(period);
      }

      void SetVolumeSlideDirection(int_t direction) override
      {
        return Delegate.SetVolumeSlideDirection(direction);
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

    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
    };

    class Format
    {
    public:
      explicit Format(Binary::View rawData)
        : RawData(rawData)
        , Source(*RawData.As<Header>())
        , Ranges(RangeChecker::Create(RawData.Size()))
      {
        // info
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        const auto title = *Source.Title.begin() == '|' && *Source.Title.rbegin() == '|'
                               ? MakeStringView(Source.Title.data() + 1, &Source.Title.back())
                               : MakeStringView(Source.Title.begin(), Source.Title.end());
        meta.SetTitle(Strings::OptimizeAscii(title));
        meta.SetProgram(DESCRIPTION);
        Strings::Array names;
        names.reserve(SAMPLES_COUNT);
        for (const auto& name : Source.SampleNames)
        {
          names.emplace_back(Strings::OptimizeAscii(name));
        }
        meta.SetStrings(names);
      }

      void ParsePositions(Builder& target) const
      {
        Positions positions;
        positions.Loop = Source.Loop;
        positions.Lines.assign(Source.Positions.begin(), Source.Positions.begin() + Source.LastPosition + 1);
        Dbg("Positions: {}, loop to {}", positions.GetSize(), positions.GetLoop());
        target.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          const Pattern& source = Source.Patterns[patIndex];
          PatternBuilder& patBuilder = target.StartPattern(patIndex);
          ParsePattern(source, patBuilder, target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        Dbg("Parse {} samples", sams.Count());
        //[bank & 7] => <offset, size>
        std::pair<std::size_t, std::size_t> regions[8] = {};
        for (std::size_t layIdx = 0, cursor = sizeof(Source); layIdx != Source.Layouts.size(); ++layIdx)
        {
          const LayoutInfo& layout = Source.Layouts[layIdx];
          const std::size_t addr = layout.Address;
          const std::size_t size = 256 * layout.Sectors;
          if (addr >= BIG_SAMPLE_ADDR && addr + size <= SAMPLES_LIMIT)
          {
            Dbg("Used bank {} at {:04x}..{:04x}", uint_t(layout.Bank), addr, addr + size);
            regions[layout.Bank & 0x07] = std::make_pair(cursor, size);
          }
          AddRange(cursor, size);
          cursor += size;
        }
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const SampleInfo& info = Source.Samples[samIdx];
          const std::size_t rawAddr = info.Start;
          const std::size_t rawLoop = info.Loop;
          if (rawAddr < BIG_SAMPLE_ADDR || rawLoop < rawAddr)
          {
            Dbg("Skip sample {}", samIdx);
            continue;
          }
          const uint_t bank = info.Bank & 0x07;
          Require(0 != regions[bank].first);
          const std::size_t sampleBase = rawAddr < SAMPLES_ADDR ? BIG_SAMPLE_ADDR : SAMPLES_ADDR;
          const std::pair<std::size_t, std::size_t>& offsetSize = regions[bank];
          const std::size_t size = std::min(SAMPLES_LIMIT - rawAddr, offsetSize.second);
          const std::size_t offset = offsetSize.first + rawAddr - sampleBase;
          if (const auto sample = GetSample(offset, size))
          {
            Dbg("Sample {}: start=#{:04x} loop=#{:04x} size=#{:04x} bank={}", samIdx, rawAddr, rawLoop, sample.Size(),
                uint_t(info.Bank));
            const std::size_t loop = info.IsLooped ? rawLoop - sampleBase : sample.Size();
            target.SetSample(samIdx, loop, sample);
          }
          else
          {
            Dbg("Empty sample {}", samIdx);
          }
        }
      }

      std::size_t GetSize() const
      {
        return Ranges->GetAffectedRange().second;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return {offsetof(Header, Patterns), sizeof(Source.Patterns)};
      }

    private:
      static void ParsePattern(const Pattern& src, PatternBuilder& patBuilder, Builder& target)
      {
        const uint_t lastLine = MAX_PATTERN_SIZE - 1;
        for (uint_t lineNum = 0; lineNum <= lastLine; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          if (lineNum != lastLine && srcLine.IsEmpty())
          {
            continue;
          }
          patBuilder.StartLine(lineNum);
          if (!ParseLine(srcLine, patBuilder, target))
          {
            break;
          }
        }
      }

      static bool ParseLine(const Pattern::Line& srcLine, PatternBuilder& patBuilder, Builder& target)
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          const Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
          if (srcChan.IsEndOfPattern())
          {
            return false;
          }
          else if (const uint8_t* newTempo = srcChan.GetNewTempo())
          {
            patBuilder.SetTempo(*newTempo);
          }
          else if (!srcChan.IsEmpty())
          {
            target.StartChannel(chanNum);
            if (srcChan.IsRest())
            {
              target.SetRest();
            }
            else if (const uint8_t* newPeriod = srcChan.GetVolumeSlidePeriod())
            {
              target.SetVolumeSlidePeriod(*newPeriod);
            }
            else
            {
              target.SetNote(srcChan.GetNote());
              target.SetSample(srcChan.GetSample());
              target.SetVolume(srcChan.GetSampleVolume());
              target.SetVolumeSlideDirection(srcChan.GetVolumeSlideDirection());
            }
          }
        }
        return true;
      }

      void AddRange(std::size_t start, std::size_t size) const
      {
        Require(Ranges->AddRange(start, size));
      }

      Binary::View GetSample(std::size_t offset, std::size_t size) const
      {
        const uint8_t* const start = RawData.As<uint8_t>() + offset;
        const uint8_t* const end = std::find(start, start + size, 0);
        return RawData.SubView(offset, end - start);
      }

    private:
      const Binary::View RawData;
      const Header& Source;
      const RangeChecker::Ptr Ranges;
    };

    const auto FORMAT =
        "?{192}"
        // layouts
        "(0080-c0 58-5f 01-80){8}"
        "?{32}"
        // title
        "20-7f{32}"
        "?{128}"
        // positions
        "00-1f{100}"
        "ff"
        "?{11}"
        // tempo???
        "02-10"
        // loop
        "00-63"
        // length
        "01-64"
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {}

      StringView GetDescription() const override
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
        if (!Format->Match(rawData))
        {
          return {};
        }
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      try
      {
        const Binary::View data(rawData);
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectionBuilder statistic(target);
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

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }  // namespace SQDigitalTracker

  Decoder::Ptr CreateSQDigitalTrackerDecoder()
  {
    return MakePtr<SQDigitalTracker::Decoder>();
  }
}  // namespace Formats::Chiptune
