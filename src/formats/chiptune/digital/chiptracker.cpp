/**
 *
 * @file
 *
 * @brief  ChipTracker support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/chiptracker.h"

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
#include <string_view.h>

#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace ChipTracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::ChipTracker");

    constexpr auto EDITOR = "Chip Tracker v{}"sv;

    // const std::size_t MAX_MODULE_SIZE = 65536;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 31;
    const uint_t CHANNELS_COUNT = 4;
    const uint_t SAMPLES_COUNT = 16;

    const uint8_t SIGNATURE[] = {'C', 'H', 'I', 'P', 'v'};

    struct Header
    {
      uint8_t Signature[5];
      std::array<char, 3> Version;
      std::array<char, 32> Title;
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      struct SampleDescr
      {
        le_uint16_t Loop;
        le_uint16_t Length;
      };
      std::array<SampleDescr, SAMPLES_COUNT> Samples;
      uint8_t Reserved[21];
      std::array<std::array<char, 8>, SAMPLES_COUNT> SampleNames;
      std::array<uint8_t, 256> Positions;
    };

    const uint_t NOTE_EMPTY = 0;
    const uint_t NOTE_BASE = 1;
    const uint_t PAUSE = 63;
    struct Note
    {
      // NNNNNNCC
      // N - note
      // C - cmd
      uint_t GetNote() const
      {
        return GetRawNote() - NOTE_BASE;
      }

      uint_t GetCommand() const
      {
        return NoteCmd & 3;
      }

      bool IsEmpty() const
      {
        return NOTE_EMPTY == GetRawNote();
      }

      bool IsPause() const
      {
        return PAUSE == GetRawNote();
      }

      uint_t GetRawNote() const
      {
        return (NoteCmd & 252) >> 2;
      }

      uint8_t NoteCmd;
    };

    using NoteRow = std::array<Note, CHANNELS_COUNT>;

    // format commands
    enum
    {
      SOFFSET = 0,
      SLIDEDN = 1,
      SLIDEUP = 2,
      SPECIAL = 3
    };

    struct NoteParam
    {
      // SSSSPPPP
      // S - sample
      // P - param
      uint_t GetParameter() const
      {
        return SampParam & 15;
      }

      uint_t GetSample() const
      {
        return (SampParam & 240) >> 4;
      }

      uint8_t SampParam;
    };

    using NoteParamRow = std::array<NoteParam, CHANNELS_COUNT>;

    struct Pattern
    {
      std::array<NoteRow, MAX_PATTERN_SIZE> Notes;
      std::array<NoteParamRow, MAX_PATTERN_SIZE> Params;
    };

    static_assert(sizeof(Header) * alignof(Header) == 512, "Invalid layout");
    static_assert(sizeof(Pattern) * alignof(Pattern) == 512, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header) + sizeof(Pattern) + 256;  // single pattern and single sample

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
      void SetSlide(int_t /*step*/) override {}
      void SetSampleOffset(uint_t /*offset*/) override {}
    };

    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
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

      void SetSlide(int_t step) override
      {
        return Delegate.SetSlide(step);
      }

      void SetSampleOffset(uint_t offset) override
      {
        return Delegate.SetSampleOffset(offset);
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
      explicit Format(Binary::View data)
        : RawData(data)
        , Source(*RawData.As<Header>())
        , Ranges(RangeChecker::Create(RawData.Size()))
        , FixedRanges(RangeChecker::Create(RawData.Size()))
      {
        // info
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetTitle(Strings::OptimizeAscii(Source.Title));
        const StringView version(Source.Version.data(), Source.Version.size());
        meta.SetProgram(Strings::Format(EDITOR, version));

        Strings::Array names;
        names.reserve(Source.SampleNames.size());
        for (const auto& name : Source.SampleNames)
        {
          names.emplace_back(Strings::OptimizeAscii(name));
        }
        meta.SetStrings(names);
      }

      void ParsePositions(Builder& target) const
      {
        Positions result;
        result.Loop = Source.Loop;
        result.Lines.assign(Source.Positions.begin(), Source.Positions.begin() + Source.Length + 1);
        Dbg("Positions: {}, loop to {}", result.GetSize(), result.GetLoop());
        target.SetPositions(std::move(result));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          PatternBuilder& patBuilder = target.StartPattern(patIndex);
          ParsePattern(patIndex, patBuilder, target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        const std::size_t patternsCount =
            1 + *std::max_element(Source.Positions.begin(), Source.Positions.begin() + Source.Length + 1);
        std::size_t sampleStart = sizeof(Header) + patternsCount * sizeof(Pattern);
        std::size_t memLeft = RawData.Size() - sampleStart;
        for (uint_t samIdx = 0; samIdx != Source.Samples.size(); ++samIdx)
        {
          const Header::SampleDescr& descr = Source.Samples[samIdx];
          const std::size_t loop = descr.Loop;
          const std::size_t size = descr.Length;
          const std::size_t availSize = std::min(memLeft, size);
          if (availSize)
          {
            if (sams.Contain(samIdx))
            {
              Dbg("Sample {}: start=#{:04x} loop=#{:04x} size=#{:04x} (avail=#{:04x})", samIdx, sampleStart, loop, size,
                  availSize);
              AddRange(sampleStart, availSize);
              target.SetSample(samIdx, loop, RawData.SubView(sampleStart, availSize));
            }
            if (size != availSize)
            {
              break;
            }
            const auto alignedSize = Math::Align<std::size_t>(size, 256);
            sampleStart += alignedSize;
            memLeft -= alignedSize;
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
      void ParsePattern(uint_t idx, PatternBuilder& patBuilder, Builder& target) const
      {
        const std::size_t patStart = sizeof(Header) + idx * sizeof(Pattern);
        const auto* src = RawData.SubView(patStart).As<Pattern>();
        Require(src != nullptr);
        // due to quite complex structure, patter lines are not optimized
        uint_t lineNum = 0;
        for (; lineNum < MAX_PATTERN_SIZE; ++lineNum)
        {
          patBuilder.StartLine(lineNum);
          const NoteRow& notes = src->Notes[lineNum];
          const NoteParamRow& params = src->Params[lineNum];
          if (!ParseLine(notes, params, patBuilder, target))
          {
            break;
          }
        }
        const std::size_t endNote = offsetof(Pattern, Notes) + lineNum * sizeof(NoteRow);
        const std::size_t endParam = offsetof(Pattern, Params) + lineNum * sizeof(NoteParamRow);
        const std::size_t patSize = std::max<std::size_t>(endNote, endParam);
        AddFixedRange(patStart, patSize);
      }

      static bool ParseLine(const NoteRow& notes, const NoteParamRow& params, PatternBuilder& patBuilder,
                            Builder& target)
      {
        bool cont = true;
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          target.StartChannel(chanNum);
          const Note& note = notes[chanNum];
          const NoteParam& param = params[chanNum];
          if (note.IsPause())
          {
            target.SetRest();
          }
          else if (!note.IsEmpty())
          {
            target.SetNote(note.GetNote());
            target.SetSample(param.GetSample());
          }
          switch (note.GetCommand())
          {
          case SOFFSET:
            if (const uint_t off = param.GetParameter())
            {
              target.SetSampleOffset(512 * off);
            }
            break;
          case SLIDEDN:
            if (const uint_t sld = param.GetParameter())
            {
              target.SetSlide(-static_cast<int8_t>(2 * sld));
            }
            break;
          case SLIDEUP:
            if (const uint_t sld = param.GetParameter())
            {
              target.SetSlide(static_cast<int8_t>(2 * sld));
            }
            break;
          case SPECIAL:
            // first channel- tempo
            if (0 == chanNum)
            {
              patBuilder.SetTempo(param.GetParameter());
            }
            // last channel- stop
            else if (3 == chanNum)
            {
              cont = false;
            }
            break;
          }
        }
        return cont;
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
      if (!header || 0 != std::memcmp(header->Signature, SIGNATURE, sizeof(SIGNATURE)))
      {
        return false;
      }
      const uint_t patternsCount =
          1 + *std::max_element(header->Positions.begin(), header->Positions.begin() + header->Length + 1);
      return sizeof(*header) + patternsCount * sizeof(Pattern) <= data.Size();
    }

    const auto DESCRIPTION = "Chip Tracker"sv;
    const auto FORMAT =
        "'C'H'I'P'v"          // uint8_t Signature[5];
        "3x2e3x"              // char Version[3];
        "20-7f{32}"           // char Name[32];
        "01-0f"               // uint8_t Tempo;
        "??"                  // len,loop
        "(?00-bb?00-bb){16}"  // samples descriptions
        "?{21}"               // uint8_t Reserved[21];
        "(20-7f{8}){16}"      // sample names
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
        return FastCheck(rawData) && Format->Match(rawData);
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
      const Binary::View data(rawData);
      if (!FastCheck(data))
      {
        return {};
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
  }  // namespace ChipTracker

  Decoder::Ptr CreateChipTrackerDecoder()
  {
    return MakePtr<ChipTracker::Decoder>();
  }
}  // namespace Formats::Chiptune
