/**
 *
 * @file
 *
 * @brief  ExtremeTracker v1.xx support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/extremetracker1.h"

#include "formats/chiptune/container.h"

#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/format.h>
#include <strings/optimize.h>
#include <tools/indices.h>
#include <tools/range_checker.h>

#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>

#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace ExtremeTracker1
  {
    const Debug::Stream Dbg("Formats::Chiptune::ExtremeTracker1");

    const auto DESCRIPTION = "Extreme Tracker v1.x"sv;
    const auto VERSION131 = "Extreme Tracker v1.31"sv;
    const auto VERSION132 = "Extreme Tracker v1.32-1.41"sv;

    const std::size_t CHANNELS_COUNT = 4;
    const std::size_t MAX_POSITIONS_COUNT = 100;
    const std::size_t MAX_SAMPLES_COUNT = 16;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_PATTERN_SIZE = 64;

    const uint64_t Z80_FREQ = 3500000;

    const uint_t TICKS_PER_CYCLE_31 = 450;
    const uint_t C_1_STEP_31 = 0x5f;
    const uint_t SAMPLES_FREQ_31 = Z80_FREQ * C_1_STEP_31 / TICKS_PER_CYCLE_31 / 256;

    const uint_t TICKS_PER_CYCLE_32 = 374;
    const uint_t C_1_STEP_32 = 0x50;
    const uint_t SAMPLES_FREQ_32 = Z80_FREQ * C_1_STEP_32 / TICKS_PER_CYCLE_32 / 256;

    /*
      Module memory map (as in original editor)
      0x0200 -> 0x7e00@50 (hdr)
      0x4000 -> 0xc000@50 (pat)
      0x3a00 -> 0xc000@51
      0x4000 -> 0xc000@53
      0x4000 -> 0xc000@54
      0x4000 -> 0xc000@56
      0x7c00 -> 0x8400@57

      Known versions:
        v1.31 - .m files (0xfc+0xbc=0x1b8 sectors)

        v1.32 - VOLUME cmd changed to GLISS, added REST cmd, .D files (same sizes)

        v1.41 -

        ??? - empty sample changed from 7ebc to 4000
    */

    struct SampleInfo
    {
      le_uint16_t Start;
      le_uint16_t Loop;
      uint8_t Page;
      uint8_t IndexInPage;
      uint8_t Blocks;
      uint8_t Padding;
      std::array<char, 8> Name;
    };

    struct MemoryDescr
    {
      uint8_t RestBlocks;
      uint8_t Page;
      uint8_t FreeStartBlock;
      uint8_t Samples;
    };

    enum Command
    {
      // real
      NONE,
      VOLUME_OR_GLISS,
      TEMPO,
      REST,
      // synthetic
      VOLUME,
      GLISS
    };

    struct Pattern
    {
      struct Line
      {
        struct Channel
        {
          uint8_t NoteCmd;
          uint8_t SampleParam;

          uint_t GetNote() const
          {
            return NoteCmd & 0x3f;
          }

          uint_t GetCmd() const
          {
            return NoteCmd >> 6;
          }

          uint_t GetSample() const
          {
            return SampleParam & 0x0f;
          }

          uint_t GetRawCmdParam() const
          {
            return SampleParam >> 4;
          }

          uint_t GetTempo() const
          {
            return GetRawCmdParam();
          }

          uint_t GetVolume() const
          {
            return GetRawCmdParam();
          }

          int_t GetGliss() const
          {
            const int_t val = SampleParam >> 4;
            return 2 * (val > 8 ? 8 - val : val);
          }

          bool IsEmpty() const
          {
            return NoteCmd + SampleParam == 0;
          }
        };

        bool IsEmpty() const
        {
          return Channels[0].IsEmpty() && Channels[1].IsEmpty() && Channels[2].IsEmpty() && Channels[3].IsEmpty();
        }

        Channel Channels[CHANNELS_COUNT];
      };

      Line Lines[MAX_PATTERN_SIZE];
    };

    struct Header
    {
      uint8_t LoopPosition;
      uint8_t Tempo;
      uint8_t Length;
      //+0x03
      std::array<char, 30> Title;
      //+0x21
      uint8_t Unknown1;
      //+0x22
      std::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
      //+0x86
      std::array<uint8_t, MAX_PATTERNS_COUNT> PatternsSizes;
      //+0xa6
      uint8_t Unknown2[2];
      //+0xa8
      std::array<MemoryDescr, 5> Pages;
      //+0xbc
      uint8_t EmptySample[0x0f];
      //+0xcb
      uint8_t Signature[44];
      //+0xf7
      uint8_t Zeroes[9];
      //+0x100
      std::array<SampleInfo, MAX_SAMPLES_COUNT> Samples;
      //+0x200
      std::array<Pattern, MAX_PATTERNS_COUNT> Patterns;
      //+0x4200
    };

    static_assert(sizeof(Pattern) * alignof(Pattern) == 0x200, "Invalid layout");
    static_assert(sizeof(Header) * alignof(Header) == 0x4200, "Invalid layout");

    const std::size_t MODULE_SIZE = 0x1b800;

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetInitialTempo(uint_t /*tempo*/) override {}
      void SetSamplesFrequency(uint_t /*freq*/) override {}
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
      void SetGliss(int_t /*gliss*/) override {}
    };

    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
        , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
      {}

      MetaBuilder& GetMetaBuilder() override
      {
        return Delegate.GetMetaBuilder();
      }

      void SetInitialTempo(uint_t tempo) override
      {
        return Delegate.SetInitialTempo(tempo);
      }

      void SetSamplesFrequency(uint_t freq) override
      {
        return Delegate.SetSamplesFrequency(freq);
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

      void SetGliss(int_t gliss) override
      {
        return Delegate.SetGliss(gliss);
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

    class VersionTraits
    {
    public:
      explicit VersionTraits(const Header& hdr)
      {
        Analyze(hdr);
      }

      auto GetEditorString() const
      {
        return IsNewVersion() ? VERSION132 : VERSION131;
      }

      uint_t GetSamplesFrequency() const
      {
        return IsNewVersion() ? SAMPLES_FREQ_32 : SAMPLES_FREQ_31;
      }

      Command DecodeCommand(uint_t cmd) const
      {
        if (cmd == VOLUME_OR_GLISS)
        {
          return IsNewVersion() ? GLISS : VOLUME;
        }
        else
        {
          return static_cast<Command>(cmd);
        }
      }

    private:
      void Analyze(const Header& hdr)
      {
        for (const auto& pat : hdr.Patterns)
        {
          Analyze(pat);
        }
      }

      void Analyze(const Pattern& pat)
      {
        for (const auto& line : pat.Lines)
        {
          Analyze(line);
        }
      }

      void Analyze(const Pattern::Line& line)
      {
        for (const auto& chan : line.Channels)
        {
          Analyze(chan);
        }
      }

      void Analyze(const Pattern::Line::Channel& chan)
      {
        switch (chan.GetCmd())
        {
        case REST:
          HasRestCmd = true;
          break;
        case VOLUME_OR_GLISS:
          GlissVolParamsMask |= 1 << chan.GetRawCmdParam();
          break;
        }
      }

      bool IsNewVersion() const
      {
        // Only newer versions has R-- command
        // TODO: research about gliss/vol parameters factors
        // VOL-0 is used
        return HasRestCmd;
      }

    private:
      bool HasRestCmd = false;
      uint_t GlissVolParamsMask = 0;
    };

    class Format
    {
    public:
      explicit Format(Binary::View rawData)
        : RawData(rawData)
        , Source(*RawData.As<Header>())
        , Version(Source)
        , Ranges(RangeChecker::Create(RawData.Size()))
      {
        // info
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetTitle(Strings::OptimizeAscii(Source.Title));
        meta.SetProgram(Version.GetEditorString());
        Strings::Array names(Source.Samples.size());
        for (uint_t idx = 0; idx != Source.Samples.size(); ++idx)
        {
          names[idx] = Strings::OptimizeAscii(Source.Samples[idx].Name);
        }
        meta.SetStrings(names);
      }

      void ParsePositions(Builder& target) const
      {
        Positions result;
        result.Loop = Source.LoopPosition;
        result.Lines.assign(Source.Positions.begin(), Source.Positions.begin() + std::max<uint_t>(Source.Length, 1));
        Dbg("Positions: {}, loop to {}", result.GetSize(), result.GetLoop());
        target.SetPositions(std::move(result));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          const Pattern& source = Source.Patterns[patIndex];
          ParsePattern(source, patIndex, target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        struct BankLayout
        {
          std::size_t FileOffset;
          std::size_t Addr;
          std::size_t End;
        };

        static const BankLayout LAYOUTS[8] = {{0, 0, 0},
                                              {0x4200, 0xc000, 0xfa00},
                                              {0, 0, 0},
                                              {0x7c00, 0xc000, 0x10000},
                                              {0xbc00, 0xc000, 0x10000},
                                              {0, 0, 0},
                                              {0xfc00, 0xc000, 0x10000},
                                              {0x13c00, 0x8400, 0x10000}};

        target.SetSamplesFrequency(Version.GetSamplesFrequency());
        Dbg("Parse {} samples", sams.Count());
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const SampleInfo& info = Source.Samples[samIdx];
          const std::size_t rawAddr = info.Start;
          const std::size_t rawLoop = info.Loop;
          const std::size_t rawEnd = rawAddr + 256 * info.Blocks;
          const BankLayout bank = LAYOUTS[info.Page & 7];
          if (0 == bank.FileOffset || 0 == info.Blocks || rawAddr < bank.Addr || rawEnd > bank.End || rawLoop < rawAddr
              || rawLoop > rawEnd)
          {
            Dbg("Skip sample {}", samIdx);
            continue;
          }
          const std::size_t size = rawEnd - rawAddr;
          const std::size_t offset = bank.FileOffset + (rawAddr - bank.Addr);
          if (const auto sample = GetSample(offset, size))
          {
            Dbg("Sample {}: start=#{:04x} loop=#{:04x} size=#{:04x} bank={}", samIdx, rawAddr, rawLoop, sample.Size(),
                uint_t(info.Page));
            const std::size_t loop = rawLoop - bank.Addr;
            target.SetSample(samIdx, loop, sample);
          }
          else
          {
            Dbg("Empty sample {}", samIdx);
          }
        }
      }

      static std::size_t GetSize()
      {
        return MODULE_SIZE;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return {offsetof(Header, Patterns), sizeof(Source.Patterns)};
      }

    private:
      void ParsePattern(const Pattern& src, uint_t patIndex, Builder& target) const
      {
        PatternBuilder& patBuilder = target.StartPattern(patIndex);
        const uint_t lastLine = Source.PatternsSizes[patIndex] - 1;
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

      bool ParseLine(const Pattern::Line& srcLine, PatternBuilder& patBuilder, Builder& target) const
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          const Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
          if (srcChan.IsEmpty())
          {
            continue;
          }
          target.StartChannel(chanNum);
          if (const uint_t note = srcChan.GetNote())
          {
            Require(note <= 48);
            target.SetNote(note - 1);
            target.SetSample(srcChan.GetSample());
          }
          switch (Version.DecodeCommand(srcChan.GetCmd()))
          {
          case NONE:
            break;
          case TEMPO:
            patBuilder.SetTempo(srcChan.GetTempo());
            break;
          case VOLUME:
            target.SetVolume(srcChan.GetVolume());
            break;
          case GLISS:
            target.SetGliss(srcChan.GetGliss());
            break;
          case REST:
            target.SetRest();
            break;
          default:
            Require(false);
            break;
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
      const VersionTraits Version;
      const RangeChecker::Ptr Ranges;
    };

    const auto FORMAT =
        // loop
        "00-63"
        // tempo
        "03-0f"
        // length
        "00-64"
        // title
        "20-7f{30}"
        // unknown
        "?"
        // positions
        "00-1f{100}"
        // pattern sizes
        "04-40{32}"
        // unknown
        "??"
        // memory
        "(00-7c 51|53|54|56|57 00|84-ff 00-10){5}"
        // unknown
        "?{15}"
        // signature
        "?{44}"
        // zeroes
        "?{9}"
        // samples. Hi addr is usually 7e-ff, but some tracks has another values (40)
        "(?? ?? 51|53|54|56|57 00-10 00-7c ? ?{8}){16}"
        // patterns
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MODULE_SIZE))
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

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
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

        auto subData = data.GetSubcontainer(0, format.GetSize());
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
  }  // namespace ExtremeTracker1

  Decoder::Ptr CreateExtremeTracker1Decoder()
  {
    return MakePtr<ExtremeTracker1::Decoder>();
  }
}  // namespace Formats::Chiptune
