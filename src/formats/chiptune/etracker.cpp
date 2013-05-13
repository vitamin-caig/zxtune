/*
Abstract:
  ETracker format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "etracker.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <indices.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::ETracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace ETracker
  {
    const std::size_t MIN_SIZE = 96;
    const std::size_t MAX_SIZE = 0x8000;
    const std::size_t MAX_POSITIONS_COUNT = 255;
    const std::size_t MIN_PATTERN_SIZE = 1;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_SAMPLES_COUNT = 32;
    const std::size_t MAX_ORNAMENTS_COUNT = 32;
    const std::size_t MAX_SAMPLE_SIZE = 256;
    const std::size_t MAX_ORNAMENT_SIZE = 256;

    /*
      Typical module structure

      Header
      Patterns data
      Ornaments
      Samples
      SamplesTable
      OrnamentsTable
      SamplesVolumeDecodeTable
      PositionsTable
      PatternsTable
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint16_t PositionsOffset;
      uint16_t PatternsOffset;
      uint16_t SamplesOffset;
      uint16_t OrnamentsOffset;
      uint16_t SamplesVolumeDecodeTable;
      uint8_t Signature[0x14];
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      boost::array<uint16_t, 6> Offsets;
    } PACK_POST;

    PACK_PRE struct RawSamplesVolumeDecodeTable
    {
      uint8_t Marker;
      uint8_t Table[1];
    } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x1e);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 12);

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, const Sample& /*sample*/) {}
      virtual void SetOrnament(uint_t /*index*/, const Ornament& /*ornament*/) {}
      virtual void SetPositions(const std::vector<PositionEntry>& /*positions*/, uint_t /*loop*/) {}
      virtual PatternBuilder& StartPattern(uint_t /*index*/)
      {
        return GetStubPatternBuilder();
      }
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetAttenuation(uint_t /*vol*/) {}
      virtual void SetSwapSampleChannels(bool /*swapSamples*/) {}
      virtual void SetEnvelope(uint_t /*value*/) {}
      virtual void SetNoise(uint_t /*type*/) {}
    };

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
        , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
        , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
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

      virtual void SetSample(uint_t index, const Sample& sample)
      {
        assert(index == 0 || UsedSamples.Contain(index));
        return Delegate.SetSample(index, sample);
      }

      virtual void SetOrnament(uint_t index, const Ornament& ornament)
      {
        assert(index == 0 || UsedOrnaments.Contain(index));
        return Delegate.SetOrnament(index, ornament);
      }

      virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop)
      {
        UsedPatterns.Clear();
        for (std::vector<PositionEntry>::const_iterator it = positions.begin(), lim = positions.end(); it != lim; ++it)
        {
          UsedPatterns.Insert(it->PatternIndex);
        }
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(positions, loop);
      }

      virtual PatternBuilder& StartPattern(uint_t index)
      {
        assert(UsedPatterns.Contain(index));
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

      virtual void SetOrnament(uint_t ornament)
      {
        UsedOrnaments.Insert(ornament);
        return Delegate.SetOrnament(ornament);
      }

      virtual void SetAttenuation(uint_t vol)
      {
        return Delegate.SetAttenuation(vol);
      }

      virtual void SetSwapSampleChannels(bool swapChannels)
      {
        return Delegate.SetSwapSampleChannels(swapChannels);
      }

      virtual void SetEnvelope(uint_t value)
      {
        return Delegate.SetEnvelope(value);
      }

      virtual void SetNoise(uint_t type)
      {
        return Delegate.SetNoise(type);
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

    class RangesMap
    {
    public:
      explicit RangesMap(std::size_t limit)
        : ServiceRanges(RangeChecker::CreateShared(limit))
        , TotalRanges(RangeChecker::CreateSimple(limit))
        , FixedRanges(RangeChecker::CreateSimple(limit))
      {
      }

      void AddService(std::size_t offset, std::size_t size) const
      {
        Require(ServiceRanges->AddRange(offset, size));
        Add(offset, size);
      }

      void AddFixed(std::size_t offset, std::size_t size) const
      {
        Require(FixedRanges->AddRange(offset, size));
        Add(offset, size);
      }

      void Add(std::size_t offset, std::size_t size) const
      {
        Dbg(" Affected range %1%..%2%", offset, offset + size);
        Require(TotalRanges->AddRange(offset, size));
      }

      std::size_t GetSize() const
      {
        return TotalRanges->GetAffectedRange().second;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return FixedRanges->GetAffectedRange();
      }
    private:
      const RangeChecker::Ptr ServiceRanges;
      const RangeChecker::Ptr TotalRanges;
      const RangeChecker::Ptr FixedRanges;
    };

    struct DecodeTable
    {
      uint_t Marker;
      struct CodeAndLen
      {
        uint_t Code;
        uint_t Len;
      };
      std::vector<CodeAndLen> Lenghts;

      DecodeTable()
        : Marker()
      {
      }

      uint_t DecodeLen(uint_t code) const
      {
        for (std::vector<CodeAndLen>::const_iterator it = Lenghts.begin(), lim = Lenghts.end(); it != lim; ++it)
        {
          if (it->Code == code)
          {
            return it->Len;
          }
        }
        return 0;
      }
    };

    class Format
    {
    public:
      Format(const Binary::TypedContainer& data)
        : Delegate(data)
        , Ranges(data.GetSize())
        , Source(*Delegate.GetField<RawHeader>(0))
      {
        Ranges.AddService(0, sizeof(Source));
        ReadSampleDecodeTable();
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(6);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Text::ETRACKER_DECODER_DESCRIPTION);
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<PositionEntry> positions;
        uint_t loop = 0;
        PositionEntry entry;
        for (std::size_t posCursor = fromLE(Source.PositionsOffset); ; ++posCursor)
        {
          Require(positions.size() <= MAX_POSITIONS_COUNT);
          const uint_t val = PeekByte(posCursor);
          if (val == 0xff)
          {
            break;
          }
          else if (val == 0xfe)
          {
            loop = positions.size();
          }
          else if (val >= 0x60)
          {
            entry.Transposition = val - 0x60;
          }
          else
          {
            Require(0 == val % 3);
            entry.PatternIndex = val / 3;
            positions.push_back(entry);
          }
        }
        Require(!positions.empty());
        builder.SetPositions(positions, loop);
        Dbg("Positions: %1% entries, loop to %2%", positions.size(), loop);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: %1% to parse", pats.Count());
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          ParsePattern(patIndex, builder);
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Dbg("Samples: %1% to parse", samples.Count());
        const std::size_t samplesTable = fromLE(Source.SamplesOffset);
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Dbg("Parse sample %1%", samIdx);
          const std::size_t samOffset = ReadWord(samplesTable, samIdx);
          Sample result;
          ParseSample(samOffset, result);
          builder.SetSample(samIdx, result);
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: %1% to parse", ornaments.Count());
        const std::size_t ornamentsTable = fromLE(Source.OrnamentsOffset);
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Dbg("Parse ornament %1%", ornIdx);
          const std::size_t ornOffset = ReadWord(ornamentsTable, ornIdx);
          Ornament result;
          ParseOrnament(ornOffset, result);
          builder.SetOrnament(ornIdx, result);
        }
      }

      std::size_t GetSize() const
      {
        return Ranges.GetSize();
      }

      RangeChecker::Range GetFixedArea() const
      {
        return Ranges.GetFixedArea();
      }
    private:
      const RawPattern& GetPattern(uint_t patIdx) const
      {
        const std::size_t patOffset = fromLE(Source.PatternsOffset) + patIdx * sizeof(RawPattern);
        Ranges.AddService(patOffset, sizeof(RawPattern));
        return *Delegate.GetField<RawPattern>(patOffset);
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const uint8_t* const data = Delegate.GetField<uint8_t>(offset);
        Require(data != 0);
        return *data;
      }

      uint16_t ReadWord(std::size_t base, std::size_t idx) const
      {
        const std::size_t size = sizeof(uint16_t);
        const std::size_t offset = base + idx * size;
        const uint16_t* const data = Delegate.GetField<uint16_t>(offset);
        Require(data != 0);
        Ranges.AddService(offset, size);
        return fromLE(*data);
      }

      struct DataCursors : public boost::array<std::size_t, 6>
      {
        explicit DataCursors(const RawPattern& src)
        {
          std::transform(src.Offsets.begin(), src.Offsets.end(), begin(), &fromLE<uint16_t>);
        }
      };

      struct ParserState
      {
        struct ChannelState
        {
          std::size_t Offset;
          uint_t Counter;

          ChannelState()
            : Offset()
            , Counter()
          {
          }

          void Skip(uint_t toSkip)
          {
            Counter -= toSkip;
          }


          static bool CompareByCounter(const ChannelState& lh, const ChannelState& rh)
          {
            return lh.Counter < rh.Counter;
          }
        };

        boost::array<ChannelState, 6> Channels;

        explicit ParserState(const DataCursors& src)
          : Channels()
        {
          for (std::size_t idx = 0; idx != src.size(); ++idx)
          {
            Channels[idx].Offset = src[idx];
          }
        }

        uint_t GetMinCounter() const
        {
          return std::min_element(Channels.begin(), Channels.end(), &ChannelState::CompareByCounter)->Counter;
        }

        void SkipLines(uint_t toSkip)
        {
          std::for_each(Channels.begin(), Channels.end(), std::bind2nd(std::mem_fun_ref(&ChannelState::Skip), toSkip));
        }
      };

      bool ParsePattern(uint_t patIndex, Builder& builder) const
      {
        const std::size_t minOffset = sizeof(Source);
        const RawPattern& pat = GetPattern(patIndex);
        const DataCursors rangesStarts(pat);
        Require(rangesStarts.end() == std::find_if(rangesStarts.begin(), rangesStarts.end(), !boost::bind(&Math::InRange<std::size_t>, _1, minOffset, Delegate.GetSize() - 1)));

        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        ParserState state(rangesStarts);
        uint_t lineIdx = 0;
        for (; lineIdx < MAX_PATTERN_SIZE; ++lineIdx)
        {
          //skip lines if required
          if (const uint_t linesToSkip = state.GetMinCounter())
          {
            state.SkipLines(linesToSkip);
            lineIdx += linesToSkip;
          }
          if (!HasLine(state))
          {
            patBuilder.Finish(std::max<uint_t>(lineIdx, MIN_PATTERN_SIZE));
            break;
          }
          patBuilder.StartLine(lineIdx);
          ParseLine(state, patBuilder, builder);
        }
        for (uint_t chanNum = 0; chanNum != rangesStarts.size(); ++chanNum)
        {
          const std::size_t start = rangesStarts[chanNum];
          if (start >= Delegate.GetSize())
          {
            Dbg("Invalid offset (%1%)", start);
          }
          else
          {
            const std::size_t stop = std::min(Delegate.GetSize(), state.Channels[chanNum].Offset + 1);
            Ranges.AddFixed(start, stop - start);
          }
        }
        return lineIdx >= MIN_PATTERN_SIZE;
      }

      bool HasLine(const ParserState& src) const
      {
        for (uint_t chan = 0; chan < 6; ++chan)
        {
          const ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter)
          {
            continue;
          }
          if (state.Offset >= Delegate.GetSize())
          {
            return false;
          }
          else if (0x51 == PeekByte(state.Offset))
          {
            return false;
          }
        }
        return true;
      }

      void ParseLine(ParserState& src, PatternBuilder& patBuilder, Builder& builder) const
      {
        for (uint_t chan = 0; chan < 6; ++chan)
        {
          ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter--)
          {
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(state, patBuilder, builder);
        }
      }

      void ParseChannel(ParserState::ChannelState& state, PatternBuilder& patBuilder, Builder& builder) const
      {
        while (state.Offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd >= 0xd2)
          {
            state.Counter = cmd - 0xd2;
            break;
          }
          else if (cmd >= 0x72)
          {
            builder.SetNote(cmd - 0x72);
          }
          else if (cmd >= 0x52)
          {
            builder.SetSample(cmd - 0x52);
          }
          else if (cmd >= 0x51)
          {
            break;
          }
          else if (cmd >= 0x50)
          {
            builder.SetRest();
          }
          else if (cmd >= 0x30)
          {
            builder.SetOrnament(cmd - 0x30);
          }
          else if (cmd >= 0x2e)
          {
            builder.SetSwapSampleChannels(cmd > 0x2e);
          }
          else if (cmd >= 0x21)
          {
            builder.SetEnvelope(cmd - 0x21);
          }
          else if (cmd >= 0x11)
          {
            builder.SetAttenuation(cmd - 0x11);
          }
          else if (cmd >= 0x0f)
          {
            builder.SetNoise(cmd != 0x0f ? 3 : cmd - 0x0f);
          }
          else
          {
            patBuilder.SetTempo(cmd + 1);
          }
        }
      }

      void ReadSampleDecodeTable()
      {
        const std::size_t start = fromLE(Source.SamplesVolumeDecodeTable);
        std::size_t cursor = start;
        SampleDecodeTable.Marker = PeekByte(cursor++);
        SampleDecodeTable.Lenghts.clear();
        while (cursor < Delegate.GetSize())
        {
          DecodeTable::CodeAndLen val;
          if ((val.Code = PeekByte(cursor++)))
          {
            val.Len = PeekByte(cursor++);
            SampleDecodeTable.Lenghts.push_back(val);
          }
          else
          {
            break;
          }
        }
        Ranges.Add(start, cursor - start);
      }

      void ParseSample(std::size_t start, Sample& dst) const
      {
        dst.Lines.clear();
        dst.Loop = ~uint_t(0);
        std::size_t cursor = start;
        Sample::Line line;
        for (uint_t devSkip = 0, volSkip = 0; ;)
        {
          if (0 == devSkip)
          {
            devSkip = 1;
            bool finished = false;
            for (;;)
            {
              const uint_t val = PeekByte(cursor++);
              if (0 == (val & 1))
              {
                if (val == 0xfe)
                {
                  dst.Loop = dst.Lines.size();
                  continue;
                }
                else if (val == 0xfc)
                {
                  finished = true;
                  break;
                }
                devSkip = 2 + (val >> 1);
              }
              else
              {
                line.ToneEnabled = 0 != (val & 16);
                line.NoiseEnabled = 0 != (val & 32);
                line.NoiseFreq = val >> 6;
                line.ToneDeviation = (((val & 0xe) >> 1) << 8) | PeekByte(cursor++);
                break;
              }
            }
            if (finished)
            {
              break;
            }
          }
          if (0 == volSkip)
          {
            const uint_t val = PeekByte(cursor++);
            if (val == SampleDecodeTable.Marker)
            {
              volSkip = PeekByte(cursor++);
              DecodeLevel(PeekByte(cursor++), line);
            }
            else if ((volSkip = SampleDecodeTable.DecodeLen(val)))
            {
              DecodeLevel(PeekByte(cursor++), line);
            }
            else
            {
              DecodeLevel(val, line);
              volSkip = 1;
            }
          }
          if (const uint_t toSkip = std::min(devSkip, volSkip))
          {
            dst.Lines.resize(dst.Lines.size() + toSkip, line);
            devSkip -= toSkip;
            volSkip -= toSkip;
          }
        }
        Ranges.Add(start, cursor - start);
        dst.Loop = std::min<uint_t>(dst.Loop, dst.Lines.size());
      }

      static void DecodeLevel(uint_t val, Sample::Line& line)
      {
        line.LeftLevel = val & 15;
        line.RightLevel = val >> 4;
      }

      void ParseOrnament(std::size_t start, Ornament& dst) const
      {
        dst.Lines.clear();
        dst.Loop = ~uint_t(0);
        std::size_t cursor = start;
        uint_t line = 0;
        for (uint_t skip = 0;;)
        {
          skip = 1;
          const uint_t val = PeekByte(cursor++);
          if (val < 0x60)
          {
            line = val;
          }
          else if (val == 0xff)
          {
            break;
          }
          else if (val == 0xfe)
          {
            dst.Loop = dst.Lines.size();
            continue;
          }
          else if (val >= 0x60)
          {
            skip = val - 0x60;
            continue;
          }
          if (skip)
          {
            dst.Lines.resize(dst.Lines.size() + skip, line);
            skip = 0;
          }
        }
        Ranges.Add(start, cursor - start);
        dst.Loop = std::min<uint_t>(dst.Loop, dst.Lines.size());
      }
    private:
      const Binary::TypedContainer& Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
      DecodeTable SampleDecodeTable;
    };

    enum AreaTypes
    {
      HEADER,
      SAMPLES_TABLE,
      ORNAMENTS_TABLE,
      SAMPLESDECODE_TABLE,
      POSITIONS_TABLE,
      PATTERNS_TABLE,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        const std::size_t samples = fromLE(header.SamplesOffset);
        const std::size_t ornaments = fromLE(header.OrnamentsOffset);
        const std::size_t samplesDecode = fromLE(header.SamplesVolumeDecodeTable);
        AddArea(SAMPLES_TABLE, samples);
        AddArea(SAMPLESDECODE_TABLE, samplesDecode);
        if (samples != ornaments && samplesDecode != ornaments)
        {
          AddArea(ORNAMENTS_TABLE, ornaments);
        }
        AddArea(POSITIONS_TABLE, fromLE(header.PositionsOffset));
        AddArea(PATTERNS_TABLE, fromLE(header.PatternsOffset));
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return GetAreaSize(HEADER) >= sizeof(RawHeader) && Undefined == GetAreaSize(END);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES_TABLE);
        //at least one record
        return size != Undefined && size >= sizeof(uint16_t);
      }

      bool CheckOrnaments() const
      {
        //at least one record or nothing
        if (Undefined == GetAreaAddress(ORNAMENTS_TABLE))
        {
          return true;
        }
        const std::size_t size = GetAreaSize(ORNAMENTS_TABLE);
        return size != Undefined && size >= sizeof(uint16_t);
      }

      bool CheckPositions() const
      {
        const std::size_t size = GetAreaSize(POSITIONS_TABLE);
        //single entry+limiter
        return size != Undefined && size >= 2;
      }

      bool CheckPatterns() const
      {
        const std::size_t size = GetAreaSize(PATTERNS_TABLE);
        return size != Undefined && size >= sizeof(RawPattern);
      }
    };

    Binary::TypedContainer CreateContainer(const Binary::Container& rawData)
    {
      return Binary::TypedContainer(rawData, std::min(rawData.Size(), MAX_SIZE));
    }

    bool FastCheck(const Binary::TypedContainer& data)
    {
      const RawHeader& hdr = *data.GetField<RawHeader>(0);
      const Areas areas(hdr, data.GetSize());
      if (!areas.CheckHeader())
      {
        return false;
      }
      if (!areas.CheckSamples())
      {
        return false;
      }
      if (!areas.CheckOrnaments())
      {
        return false;
      }
      if (!areas.CheckPositions())
      {
        return false;
      }
      if (!areas.CheckPatterns())
      {
        return false;
      }
      return true;
    }

    bool FastCheck(const Binary::Container& rawData)
    {
      const Binary::TypedContainer data = CreateContainer(rawData);
      return FastCheck(data);
    }

    const std::string FORMAT(
      "(?00-7f)"
      "(?00-7f)"
      "(?00-7f)"
      "(?00-7f)"
      "(?00-7f)"
      "'E'T'r'a'c'k'e'r' '('C')' 'B'Y' 'E'S'I'."
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
        return Text::ETRACKER_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData) && FastCheck(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const Binary::TypedContainer data = CreateContainer(rawData);

      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectingBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

        const Binary::Container::Ptr subData = rawData.GetSubcontainer(0, format.GetSize());
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
  }// namespace ETracker

  Decoder::Ptr CreateETrackerDecoder()
  {
    return boost::make_shared<ETracker::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats
