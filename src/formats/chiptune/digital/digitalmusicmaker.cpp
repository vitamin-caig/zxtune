/**
 *
 * @file
 *
 * @brief  DigitalMusicMaker support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/digital/digitalmusicmaker.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
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
#include <map>

namespace Formats::Chiptune
{
  namespace DigitalMusicMaker
  {
    const Debug::Stream Dbg("Formats::Chiptune::DigitalMusicMaker");

    const Char DESCRIPTION[] = "Digital Music Maker v1.x";

    // const std::size_t MAX_POSITIONS_COUNT = 0x32;
    // const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 24;
    const std::size_t CHANNELS_COUNT = 3;
    const std::size_t SAMPLES_COUNT = 16;  // 15 really

    const std::size_t SAMPLES_ADDR = 0xc000;

    struct Pattern
    {
      struct Line
      {
        struct Channel
        {
          uint8_t NoteCommand;
          uint8_t SampleParam;
          uint8_t Effect;
        };

        Channel Channels[CHANNELS_COUNT];
      };

      Line Lines[1];  // at least 1
    };

    struct SampleInfo
    {
      std::array<char, 9> Name;
      le_uint16_t Start;
      uint8_t Bank;
      le_uint16_t Limit;
      le_uint16_t Loop;
    };

    struct MixedLine
    {
      Pattern::Line::Channel Mixin;
      uint8_t Period;
    };

    struct Header
    {
      //+0
      std::array<le_uint16_t, 6> EndOfBanks;
      //+0x0c
      uint8_t PatternSize;
      //+0x0d
      uint8_t Padding1;
      //+0x0e
      std::array<uint8_t, 0x32> Positions;
      //+0x40
      uint8_t Tempo;
      //+0x41
      uint8_t Loop;
      //+0x42
      uint8_t Padding2;
      //+0x43
      uint8_t Length;
      //+0x44
      uint8_t HeaderSizeSectors;
      //+0x45
      MixedLine Mixings[5];
      //+0x59
      uint8_t Padding3;
      //+0x5a
      SampleInfo SampleDescriptions[SAMPLES_COUNT];
      //+0x15a
      uint8_t Padding4[4];
      //+0x15e
      // patterns starts here
    };

    static_assert(sizeof(MixedLine) * alignof(MixedLine) == 4, "Invalid layout");
    static_assert(sizeof(SampleInfo) * alignof(SampleInfo) == 16, "Invalid layout");
    static_assert(sizeof(Header) * alignof(Header) == 0x15e, "Invalid layout");
    static_assert(sizeof(Pattern::Line) * alignof(Pattern::Line) == 9, "Invalid layout");

    const std::size_t MODULE_SIZE = sizeof(Header);

    enum
    {
      NOTE_BASE = 1,
      NO_DATA = 70,
      REST_NOTE = 61,
      SET_TEMPO = 62,
      SET_FREQ_FLOAT = 63,
      SET_VIBRATO = 64,
      SET_ARPEGGIO = 65,
      SET_SLIDE = 66,
      SET_DOUBLE = 67,
      SET_ATTACK = 68,
      SET_DECAY = 69,

      FX_FLOAT_UP = 1,
      FX_FLOAT_DN = 2,
      FX_VIBRATO = 3,
      FX_ARPEGGIO = 4,
      FX_STEP_UP = 5,
      FX_STEP_DN = 6,
      FX_DOUBLE = 7,
      FX_ATTACK = 8,
      FX_DECAY = 9,
      FX_MIX = 10,
      FX_DISABLE = 15
    };

    class StubChannelBuilder : public ChannelBuilder
    {
    public:
      void SetRest() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetSample(uint_t /*sample*/) override {}
      void SetVolume(uint_t /*volume*/) override {}

      void SetFloat(int_t /*direction*/) override {}
      void SetFloatParam(uint_t /*step*/) override {}
      void SetVibrato() override {}
      void SetVibratoParams(uint_t /*step*/, uint_t /*period*/) override {}
      void SetArpeggio() override {}
      void SetArpeggioParams(uint_t /*step*/, uint_t /*period*/) override {}
      void SetSlide(int_t /*direction*/) override {}
      void SetSlideParams(uint_t /*step*/, uint_t /*period*/) override {}
      void SetDoubleNote() override {}
      void SetDoubleNoteParam(uint_t /*period*/) override {}
      void SetVolumeAttack() override {}
      void SetVolumeAttackParams(uint_t /*limit*/, uint_t /*period*/) override {}
      void SetVolumeDecay() override {}
      void SetVolumeDecayParams(uint_t /*limit*/, uint_t /*period*/) override {}
      void SetMixSample(uint_t /*idx*/) override {}
      void SetNoEffects() override {}
    };

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      void SetInitialTempo(uint_t /*tempo*/) override {}
      void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::View /*sample*/) override {}
      std::unique_ptr<ChannelBuilder> SetSampleMixin(uint_t /*index*/, uint_t /*period*/) override
      {
        return std::unique_ptr<ChannelBuilder>(new StubChannelBuilder());
      }
      void SetPositions(Positions /*positions*/) override {}
      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }
      std::unique_ptr<ChannelBuilder> StartChannel(uint_t /*index*/) override
      {
        return std::unique_ptr<ChannelBuilder>(new StubChannelBuilder());
      }
    };

    // Do not collect samples info due to high complexity of intermediate layers
    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, PATTERNS_COUNT - 1)
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

      std::unique_ptr<ChannelBuilder> SetSampleMixin(uint_t index, uint_t period) override
      {
        return Delegate.SetSampleMixin(index, period);
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

      std::unique_ptr<ChannelBuilder> StartChannel(uint_t index) override
      {
        return Delegate.StartChannel(index);
      }

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

    private:
      Builder& Delegate;
      Indices UsedPatterns;
    };

    class Format
    {
    public:
      explicit Format(Binary::View rawData)
        : RawData(rawData)
        , Source(*RawData.As<Header>())
        , Ranges(RangeChecker::Create(RawData.Size()))
        , FixedRanges(RangeChecker::Create(RawData.Size()))
      {
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetProgram(DESCRIPTION);
        Strings::Array names(SAMPLES_COUNT);
        for (uint_t samIdx = 1; samIdx < SAMPLES_COUNT; ++samIdx)
        {
          const SampleInfo& srcSample = Source.SampleDescriptions[samIdx - 1];
          if (srcSample.Name[0] != '.')
          {
            names[samIdx] = Strings::OptimizeAscii(srcSample.Name);
          }
        }
        meta.SetStrings(names);
      }

      void ParsePositions(Builder& target) const
      {
        Positions positions;
        positions.Loop = Source.Loop;
        positions.Lines.assign(Source.Positions.begin(), Source.Positions.begin() + Source.Length + 1);
        Dbg("Positions: {}, loop to {}", positions.GetSize(), positions.GetLoop());
        target.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          ParsePattern(patIndex, target);
        }
      }

      void ParseMixins(Builder& target) const
      {
        // disable UB with out-of-bound array access
        const MixedLine* const mixings = Source.Mixings;
        // big mixins amount support
        const uint_t availMixingsCount = 64;
        const uint_t maxMixingsCount = (RawData.Size() - offsetof(Header, Mixings)) / sizeof(MixedLine);
        for (uint_t mixIdx = 0, mixLimit = std::min(availMixingsCount, maxMixingsCount); mixIdx < mixLimit; ++mixIdx)
        {
          Dbg("Parse mixin {}", mixIdx);
          const MixedLine& src = mixings[mixIdx];
          const std::unique_ptr<ChannelBuilder> dst = target.SetSampleMixin(mixIdx, src.Period);
          ParseChannel(src.Mixin, *dst);
        }
      }

      void ParseSamples(Builder& target) const
      {
        const bool is4bitSamples = true;  // TODO: detect
        const std::size_t limit = RawData.Size();
        std::map<uint_t, Binary::View> regions;
        for (std::size_t layIdx = 0, lastData = 256 * Source.HeaderSizeSectors; layIdx < Source.EndOfBanks.size();
             ++layIdx)
        {
          static const uint_t BANKS[] = {0, 1, 3, 4, 6, 7};

          const uint_t bankNum = BANKS[layIdx];
          const std::size_t bankEnd = Source.EndOfBanks[layIdx];
          Require(bankEnd >= SAMPLES_ADDR);
          if (bankEnd == SAMPLES_ADDR)
          {
            Dbg("Skipping empty bank #{:02x} (end=#{:04x})", bankNum, bankEnd);
            continue;
          }
          const std::size_t bankSize = bankEnd - SAMPLES_ADDR;
          const auto alignedBankSize = Math::Align<std::size_t>(bankSize, 256);
          if (is4bitSamples)
          {
            const std::size_t realSize = 256 * (1 + alignedBankSize / 512);
            Require(lastData + realSize <= limit);
            regions.emplace(bankNum, RawData.SubView(lastData, realSize));
            Dbg("Added unpacked bank #{:02x} (end=#{:04x}, size=#{:04x}) offset=#{:05x}", bankNum, bankEnd, realSize,
                lastData);
            AddRange(lastData, realSize);
            lastData += realSize;
          }
          else
          {
            Require(lastData + alignedBankSize <= limit);
            regions.emplace(bankNum, RawData.SubView(lastData, alignedBankSize));
            Dbg("Added bank #{:02x} (end=#{:04x}, size=#{:04x}) offset=#{:05x}", bankNum, bankEnd, alignedBankSize,
                lastData);
            AddRange(lastData, alignedBankSize);
            lastData += alignedBankSize;
          }
        }

        for (uint_t samIdx = 1; samIdx < SAMPLES_COUNT; ++samIdx)
        {
          const SampleInfo& srcSample = Source.SampleDescriptions[samIdx - 1];
          if (srcSample.Name[0] == '.')
          {
            Dbg("No sample {}", samIdx);
            continue;
          }
          const std::size_t sampleStart = srcSample.Start;
          const std::size_t sampleEnd = srcSample.Limit;
          std::size_t sampleLoop = srcSample.Loop;
          Dbg("Processing sample {} (bank #{:02x} #{:04x}..#{:04x} loop #{:04x})", samIdx, uint_t(srcSample.Bank),
              sampleStart, sampleEnd, sampleLoop);
          Require(sampleStart >= SAMPLES_ADDR && sampleStart <= sampleEnd);
          if (sampleLoop < sampleStart)
          {
            sampleLoop = sampleEnd;
          }
          Require((srcSample.Bank & 0xf8) == 0x50);
          const uint_t bankIdx = srcSample.Bank & 0x07;
          const auto& bankData = regions.at(bankIdx);
          Require(!!bankData);
          const std::size_t offsetInBank = sampleStart - SAMPLES_ADDR;
          const std::size_t limitInBank = sampleEnd - SAMPLES_ADDR;
          const std::size_t sampleSize = limitInBank - offsetInBank;
          const uint_t multiplier = is4bitSamples ? 2 : 1;
          Require(limitInBank <= multiplier * bankData.Size());
          const std::size_t realSampleSize = sampleSize >= 12 ? (sampleSize - 12) : sampleSize;
          if (const auto content = bankData.SubView(offsetInBank / multiplier, realSampleSize / multiplier))
          {
            const std::size_t loop = sampleLoop - sampleStart;
            target.SetSample(samIdx, loop, content);
          }
        }
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
      void ParsePattern(uint_t idx, Builder& target) const
      {
        const uint_t patternSize = Source.PatternSize;
        const std::size_t patStart = sizeof(Source) + sizeof(Pattern::Line) * idx * patternSize;
        const std::size_t limit = RawData.Size();
        Require(patStart < limit);
        const uint_t availLines = (limit - patStart) / sizeof(Pattern::Line);
        PatternBuilder& patBuilder = target.StartPattern(idx);
        const auto& src = *RawData.SubView(patStart).As<Pattern>();
        uint_t lineNum = 0;
        for (const uint_t lines = std::min(availLines, patternSize); lineNum < lines; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          patBuilder.StartLine(lineNum);
          ParseLine(srcLine, patBuilder, target);
        }
        const std::size_t patSize = lineNum * sizeof(Pattern::Line);
        AddFixedRange(patStart, patSize);
      }

      static void ParseLine(const Pattern::Line& line, PatternBuilder& patBuilder, Builder& target)
      {
        for (uint_t chanNum = 0; chanNum < CHANNELS_COUNT; ++chanNum)
        {
          const Pattern::Line::Channel& srcChan = line.Channels[chanNum];
          std::unique_ptr<ChannelBuilder> dstChan = target.StartChannel(chanNum);
          ParseChannel(srcChan, *dstChan);
          if (srcChan.NoteCommand == SET_TEMPO && srcChan.SampleParam)
          {
            patBuilder.SetTempo(srcChan.SampleParam);
          }
        }
      }

      static void ParseChannel(const Pattern::Line::Channel& srcChan, ChannelBuilder& dstChan)
      {
        const uint_t note = srcChan.NoteCommand;
        if (NO_DATA == note)
        {
          return;
        }
        if (note < SET_TEMPO)
        {
          if (note)
          {
            if (note != REST_NOTE)
            {
              dstChan.SetNote(note - NOTE_BASE);
            }
            else
            {
              dstChan.SetRest();
            }
          }
          const uint_t params = srcChan.SampleParam;
          if (const uint_t sample = params >> 4)
          {
            dstChan.SetSample(sample);
          }
          if (const uint_t volume = params & 15)
          {
            dstChan.SetVolume(volume);
          }
          switch (srcChan.Effect)
          {
          case 0:
            break;
          case FX_FLOAT_UP:
            dstChan.SetFloat(1);
            break;
          case FX_FLOAT_DN:
            dstChan.SetFloat(-1);
            break;
          case FX_VIBRATO:
            dstChan.SetVibrato();
            break;
          case FX_ARPEGGIO:
            dstChan.SetArpeggio();
            break;
          case FX_STEP_UP:
            dstChan.SetSlide(1);
            break;
          case FX_STEP_DN:
            dstChan.SetSlide(-1);
            break;
          case FX_DOUBLE:
            dstChan.SetDoubleNote();
            break;
          case FX_ATTACK:
            dstChan.SetVolumeAttack();
            break;
          case FX_DECAY:
            dstChan.SetVolumeDecay();
            break;
          case FX_DISABLE:
            dstChan.SetNoEffects();
            break;
          default:
          {
            const uint_t mixNum = srcChan.Effect - FX_MIX;
            // according to player there can be up to 64 mixins (with enabled 4)
            dstChan.SetMixSample(mixNum % 64);
          }
          break;
          }
        }
        else
        {
          switch (note)
          {
          case SET_TEMPO:
            break;
          case SET_FREQ_FLOAT:
            dstChan.SetFloatParam(srcChan.SampleParam);
            break;
          case SET_VIBRATO:
            dstChan.SetVibratoParams(srcChan.SampleParam, srcChan.Effect);
            break;
          case SET_ARPEGGIO:
            dstChan.SetArpeggioParams(srcChan.SampleParam, srcChan.Effect);
            break;
          case SET_SLIDE:
            dstChan.SetSlideParams(srcChan.SampleParam, srcChan.Effect);
            break;
          case SET_DOUBLE:
            dstChan.SetDoubleNoteParam(srcChan.SampleParam);
            break;
          case SET_ATTACK:
            dstChan.SetVolumeAttackParams(srcChan.SampleParam & 15, srcChan.Effect);
            break;
          case SET_DECAY:
            dstChan.SetVolumeDecayParams(srcChan.SampleParam & 15, srcChan.Effect);
            break;
          }
        }
      }

      void AddRange(std::size_t start, std::size_t size) const
      {
        Require(Ranges->AddRange(start, size));
      }

      void AddFixedRange(std::size_t start, std::size_t size) const
      {
        Require(FixedRanges->AddRange(start, size));
        Require(Ranges->AddRange(start, size));
      }

    private:
      const Binary::View RawData;
      const Header& Source;
      const RangeChecker::Ptr Ranges;
      const RangeChecker::Ptr FixedRanges;
    };

    bool FastCheck(Binary::View data)
    {
      const auto* header = data.As<Header>();
      return header
             && (header->PatternSize == 64 || header->PatternSize == 48 || header->PatternSize == 32
                 || header->PatternSize == 24);
    }

    const auto FORMAT =
        // bank ends
        "(?c0-ff){6}"
        // pat size: 64,48,32,24
        "%0xxxx000 ?"
        // positions
        "(00-17){50}"
        // tempo (3..30)
        "03-1e"
        // loop position
        "00-32 ?"
        // length
        "01-32"
        // base size
        "02-38"
        ""_sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MODULE_SIZE))
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
        const Binary::View data(rawData);
        if (!Format->Match(data))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        Builder& stub = GetStubBuilder();
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
        format.ParseMixins(target);
        format.ParseSamples(target);

        auto subData = rawData.GetSubcontainer(0, format.GetSize());
        const auto fixedRange = format.GetFixedArea();
        return CreateCalculatingCrcContainer(std::move(subData), fixedRange.first,
                                             fixedRange.second - fixedRange.first);
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
  }  // namespace DigitalMusicMaker

  Decoder::Ptr CreateDigitalMusicMakerDecoder()
  {
    return MakePtr<DigitalMusicMaker::Decoder>();
  }
}  // namespace Formats::Chiptune
