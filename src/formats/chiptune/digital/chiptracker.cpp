/**
* 
* @file
*
* @brief  ChipTracker support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "chiptracker.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <make_ptr.h>
#include <range_checker.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/format.h>
//std includes
#include <array>
#include <cstring>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace ChipTracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::ChipTracker");

    const std::size_t MAX_MODULE_SIZE = 65536;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 31;
    const uint_t CHANNELS_COUNT = 4;
    const uint_t SAMPLES_COUNT = 16;

    const uint8_t SIGNATURE[] = {'C', 'H', 'I', 'P', 'v'};
    
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Header
    {
      uint8_t Signature[5];
      char Version[3];
      char Title[32];
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      PACK_PRE struct SampleDescr
      {
        uint16_t Loop;
        uint16_t Length;
      } PACK_POST;
      std::array<SampleDescr, SAMPLES_COUNT> Samples;
      uint8_t Reserved[21];
      std::array<char[8], SAMPLES_COUNT> SampleNames;
      std::array<uint8_t, 256> Positions;
    } PACK_POST;

    const uint_t NOTE_EMPTY = 0;
    const uint_t NOTE_BASE = 1;
    const uint_t PAUSE = 63;
    PACK_PRE struct Note
    {
      //NNNNNNCC
      //N - note
      //C - cmd
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
    } PACK_POST;

    typedef std::array<Note, CHANNELS_COUNT> NoteRow;

    //format commands
    enum
    {
      SOFFSET = 0,
      SLIDEDN = 1,
      SLIDEUP = 2,
      SPECIAL = 3
    };

    PACK_PRE struct NoteParam
    {
      // SSSSPPPP
      //S - sample
      //P - param
      uint_t GetParameter() const
      {
        return SampParam & 15;
      }

      uint_t GetSample() const
      {
        return (SampParam & 240) >> 4;
      }

      uint8_t SampParam;
    } PACK_POST;

    typedef std::array<NoteParam, CHANNELS_COUNT> NoteParamRow;

    PACK_PRE struct Pattern
    {
      std::array<NoteRow, MAX_PATTERN_SIZE> Notes;
      std::array<NoteParamRow, MAX_PATTERN_SIZE> Params;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(Header) == 512, "Invalid layout");
    static_assert(sizeof(Pattern) == 512, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header) + sizeof(Pattern) + 256;//single pattern and single sample

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetInitialTempo(uint_t /*tempo*/) override {}
      void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::Data::Ptr /*content*/) override {}
      void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) override {}

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
      {
      }

      MetaBuilder& GetMetaBuilder() override
      {
        return Delegate.GetMetaBuilder();
      }

      void SetInitialTempo(uint_t tempo) override
      {
        return Delegate.SetInitialTempo(tempo);
      }

      void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr data) override
      {
        return Delegate.SetSample(index, loop, data);
      }

      void SetPositions(const std::vector<uint_t>& positions, uint_t loop) override
      {
        UsedPatterns.Assign(positions.begin(), positions.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(positions, loop);
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
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*static_cast<const Header*>(RawData.Start()))
        , Ranges(RangeChecker::Create(RawData.Size()))
        , FixedRanges(RangeChecker::Create(RawData.Size()))
      {
        //info
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetTitle(FromCharArray(Source.Title));
        meta.SetProgram(Strings::Format(Text::CHIPTRACKER_EDITOR, FromCharArray(Source.Version)));

        Strings::Array names(Source.SampleNames.size());
        std::transform(Source.SampleNames.begin(), Source.SampleNames.end(), names.begin(), &FromCharArray<8>);
        meta.SetStrings(names);
      }

      void ParsePositions(Builder& target) const
      {
        const std::vector<uint_t> positions(Source.Positions.begin(), Source.Positions.begin() + Source.Length + 1);
        target.SetPositions(positions, Source.Loop);
        Dbg("Positions: %1%, loop to %2%", positions.size(), unsigned(Source.Loop));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          PatternBuilder& patBuilder = target.StartPattern(patIndex);
          ParsePattern(patIndex, patBuilder, target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        const std::size_t patternsCount = 1 + *std::max_element(Source.Positions.begin(), Source.Positions.begin() + Source.Length + 1);
        std::size_t sampleStart = sizeof(Header) + patternsCount * sizeof(Pattern);
        std::size_t memLeft = RawData.Size() - sampleStart;
        for (uint_t samIdx = 0; samIdx != Source.Samples.size(); ++samIdx)
        {
          const Header::SampleDescr& descr = Source.Samples[samIdx];
          const std::size_t loop = fromLE(descr.Loop);
          const std::size_t size = fromLE(descr.Length);
          const std::size_t availSize = std::min(memLeft, size);
          if (availSize)
          {
            if (sams.Contain(samIdx))
            {
              Dbg("Sample %1%: start=#%2$04x loop=#%3$04x size=#%4$04x (avail=#%5$04x)", 
                samIdx, sampleStart, loop, size, availSize);
              AddRange(sampleStart, availSize);
              const Binary::Data::Ptr content = RawData.GetSubcontainer(sampleStart, availSize);
              target.SetSample(samIdx, loop, content);
            }
            if (size != availSize)
            {
              break;
            }
            const std::size_t alignedSize = Math::Align<std::size_t>(size, 256);
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
        const Binary::TypedContainer data(RawData);
        const Pattern* const src = data.GetField<Pattern>(patStart);
        Require(src != nullptr);
        //due to quite complex structure, patter lines are not optimized
        uint_t lineNum = 0;
        for (; lineNum < MAX_PATTERN_SIZE ; ++lineNum)
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

      static bool ParseLine(const NoteRow& notes, const NoteParamRow& params, PatternBuilder& patBuilder, Builder& target)
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
            //first channel- tempo
            if (0 == chanNum)
            {
              patBuilder.SetTempo(param.GetParameter());
            }
            //last channel- stop
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
      const Binary::Container& RawData;
      const Header& Source;
      const RangeChecker::Ptr Ranges;
      const RangeChecker::Ptr FixedRanges;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size(rawData.Size());
      if (sizeof(Header) > size)
      {
        return false;
      }
      const Header* const header(static_cast<const Header*>(rawData.Start()));
      if (0 != std::memcmp(header->Signature, SIGNATURE, sizeof(SIGNATURE)))
      {
        return false;
      }
      const uint_t patternsCount = 1 + *std::max_element(header->Positions.begin(), header->Positions.begin() + header->Length + 1);
      if (sizeof(*header) + patternsCount * sizeof(Pattern) > size)
      {
        return false;
      }
      return true;
    }

    const std::string FORMAT(
      "'C'H'I'P'v"    // uint8_t Signature[5];
      "3x2e3x"        // char Version[3];
      "20-7f{32}"     // char Name[32];
      "01-0f"         // uint8_t Tempo;
      "??"            // len,loop
      "(?00-bb?00-bb){16}"//samples descriptions
      "?{21}"         // uint8_t Reserved[21];
      "(20-7f{8}){16}"// sample names
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      String GetDescription() const override
      {
        return Text::CHIPTRACKER_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(const Binary::Container& rawData) const override
      {
        return FastCheck(rawData) && Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
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

        Require(format.GetSize() >= MIN_SIZE);
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

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace ChipTracker

  Decoder::Ptr CreateChipTrackerDecoder()
  {
    return MakePtr<ChipTracker::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
