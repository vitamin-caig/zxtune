/*
Abstract:
  SQ-Tracker (Digital) format description implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "sqdigitaltracker.h"
#include "formats/chiptune/container.h"
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
#include <strings/format.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::SQDigitalTracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace SQDigitalTracker
  {
    const std::size_t MAX_MODULE_SIZE = 0x4400 + 8 * 0x4000;
    const std::size_t MAX_POSITIONS_COUNT = 100;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 32;
    const std::size_t CHANNELS_COUNT = 4;
    const std::size_t SAMPLES_COUNT = 16;

    const std::size_t BIG_SAMPLE_ADDR = 0x8000;
    const std::size_t SAMPLES_ADDR = 0xc000;
    const std::size_t SAMPLES_LIMIT = 0x10000;

    const uint8_t SIGNATURE[] = {'C', 'H', 'I', 'P', 'v'};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Pattern
    {
      PACK_PRE struct Line
      {
        PACK_PRE struct Channel
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
            return 62 == NoteCmd
              ? &SampleEffect : 0;
          }

          bool IsEndOfPattern() const
          {
            return 63 == NoteCmd;
          }

          const uint8_t* GetVolumeSlidePeriod() const
          {
            return 64 == NoteCmd
              ? &SampleEffect : 0;
          }

          int_t GetVolumeSlideDirection() const
          {
            //0 - no slide,
            //1 - -1
            //2 - +1
            const int_t val = NoteCmd >> 6;
            return val ? (val - 1) : val;
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
        } PACK_POST;

        Channel Channels[CHANNELS_COUNT];

        bool IsEmpty() const
        {
          return Channels[0].IsEmpty()
              && Channels[1].IsEmpty()
              && Channels[2].IsEmpty()
              && Channels[3].IsEmpty()
          ;
        }
      } PACK_POST;

      Line Lines[MAX_PATTERN_SIZE];
    } PACK_POST;

    PACK_PRE struct SampleInfo
    {
      uint16_t Start;
      uint16_t Loop;
      uint8_t IsLooped;
      uint8_t Bank;
      uint8_t Padding[2];
    } PACK_POST;

    PACK_PRE struct LayoutInfo
    {
      uint16_t Address;
      uint8_t Bank;
      uint8_t Sectors;
    } PACK_POST;

    PACK_PRE struct Header
    {
      //+0
      uint8_t SamplesData[0x80];
      //+0x80
      uint8_t Banks[0x40];
      //+0xc0
      boost::array<LayoutInfo, 8> Layouts;
      //+0xe0
      uint8_t Padding1[0x20];
      //+0x100
      boost::array<char, 0x20> Title;
      //+0x120
      SampleInfo Samples[SAMPLES_COUNT];
      //+0x1a0
      boost::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
      //+0x204
      uint8_t PositionsLimit;
      //+0x205
      uint8_t Padding2[0xb];
      //+0x210
      uint8_t Tempo;
      //+0x211
      uint8_t Loop;
      //+0x212
      uint8_t Length;
      //+0x213
      uint8_t Padding3[0xed];
      //+0x300
      boost::array<uint8_t, 8> SampleNames[SAMPLES_COUNT];
      //+0x380
      uint8_t Padding4[0x80];
      //+0x400
      boost::array<Pattern, PATTERNS_COUNT> Patterns;
      //+0x4400
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Header) == 0x4400);

    const std::size_t MIN_SIZE = sizeof(Header);

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }

      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::Data::Ptr /*content*/) {}
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}

      virtual PatternBuilder& StartPattern(uint_t /*index*/)
      {
        return GetStubPatternBuilder();
      }

      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetVolume(uint_t /*volume*/) {}
      virtual void SetVolumeSlidePeriod(uint_t /*period*/) {}
      virtual void SetVolumeSlideDirection(int_t /*direction*/) {}
    };

    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, PATTERNS_COUNT - 1)
        , UsedSamples(0, SAMPLES_COUNT - 1)
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

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        UsedPatterns.Assign(positions.begin(), positions.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(positions, loop);
      }

      virtual PatternBuilder& StartPattern(uint_t index)
      {
        return Delegate.StartPattern(index);
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

      virtual void SetVolume(uint_t volume)
      {
        return Delegate.SetVolume(volume);
      }

      virtual void SetVolumeSlidePeriod(uint_t period)
      {
        return Delegate.SetVolumeSlidePeriod(period);
      }

      virtual void SetVolumeSlideDirection(int_t direction)
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
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*safe_ptr_cast<const Header*>(RawData.Start()))
        , Ranges(RangeChecker::Create(RawData.Size()))
      {
        //info
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        const String title = *Source.Title.begin() == '|' && *Source.Title.rbegin() == '|'
          ? String(Source.Title.begin() + 1, Source.Title.end() - 1)
          : String(Source.Title.begin(), Source.Title.end());
        meta.SetTitle(title);
        meta.SetProgram(Text::SQDIGITALTRACKER_DECODER_DESCRIPTION);
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
          const Pattern& source = Source.Patterns[patIndex];
          PatternBuilder& patBuilder = target.StartPattern(patIndex);
          ParsePattern(source, patBuilder, target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        Dbg("Parse %1% samples", sams.Count());
        //[bank & 7] => <offset, size>
        std::pair<std::size_t, std::size_t> regions[8] = {};
        for (std::size_t layIdx = 0, cursor = sizeof(Source); layIdx != Source.Layouts.size(); ++layIdx)
        {
          const LayoutInfo& layout = Source.Layouts[layIdx];
          const std::size_t addr = fromLE(layout.Address);
          const std::size_t size = 256 * layout.Sectors;
          if (addr >= BIG_SAMPLE_ADDR && addr + size <= SAMPLES_LIMIT)
          {
            Dbg("Used bank %1% at %2$04x..%3$04x", uint_t(layout.Bank), addr, addr + size);
            regions[layout.Bank & 0x07] = std::make_pair(cursor, size);
          }
          AddRange(cursor, size);
          cursor += size;
        }
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const SampleInfo& info = Source.Samples[samIdx];
          const std::size_t rawAddr = fromLE(info.Start);
          const std::size_t rawLoop = fromLE(info.Loop);
          if (rawAddr < BIG_SAMPLE_ADDR
           || rawLoop < rawAddr)
          {
            Dbg("Skip sample %1%", samIdx);
            continue;
          }
          const uint_t bank = info.Bank & 0x07;
          Require(regions[bank].first);
          const std::size_t sampleBase = rawAddr < SAMPLES_ADDR ? BIG_SAMPLE_ADDR : SAMPLES_ADDR;
          const std::pair<std::size_t, std::size_t>& offsetSize = regions[bank];
          const std::size_t size = std::min(SAMPLES_LIMIT - rawAddr, offsetSize.second);
          const std::size_t offset = offsetSize.first + rawAddr - sampleBase;
          if (const Binary::Data::Ptr sample = GetSample(offset, size))
          {
            Dbg("Sample %1%: start=#%2$04x loop=#%3$04x size=#%4$04x bank=%5%", 
              samIdx, rawAddr, rawLoop, sample->Size(), uint_t(info.Bank));
            const std::size_t loop = info.IsLooped ? rawLoop - sampleBase : sample->Size();
            target.SetSample(samIdx, loop, sample);
          }
          else
          {
            Dbg("Empty sample %1%", samIdx);
          }
        }
      }

      std::size_t GetSize() const
      {
        return Ranges->GetAffectedRange().second;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return RangeChecker::Range(offsetof(Header, Patterns), sizeof(Source.Patterns));
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

      Binary::Data::Ptr GetSample(std::size_t offset, std::size_t size) const
      {
        const uint8_t* const start = static_cast<const uint8_t*>(RawData.Start()) + offset;
        const uint8_t* const end = std::find(start, start + size, 0);
        return RawData.GetSubcontainer(offset, end - start);
      }
    private:
      const Binary::Container& RawData;
      const Header& Source;
      const RangeChecker::Ptr Ranges;
    };

    const std::string FORMAT(
      "?{192}"
      //layouts
      "(0080-c0 58-5f 01-80){8}"
      "?{32}"
      //title
      "20-7f{32}"
      "?{128}"
      //positions
      "00-1f{100}"
      "ff"
      "?{11}"
      //tempo???
      "02-10"
      //loop
      "00-63"
      //length
      "01-64"
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
        return Text::SQDIGITALTRACKER_DECODER_DESCRIPTION;
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
  }//namespace SQDigitalTracker

  Decoder::Ptr CreateSQDigitalTrackerDecoder()
  {
    return boost::make_shared<SQDigitalTracker::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
