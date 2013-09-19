/*
Abstract:
  SoundTracker compiled format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtracker_detail.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <iterator.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/end.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::SoundTrackerCompiled");
}

namespace Formats
{
namespace Chiptune
{
  namespace SoundTrackerCompiled
  {
    using namespace SoundTracker;

    const std::size_t MIN_SIZE = 128;
    const std::size_t MAX_SIZE = 0x2600;

    /*
      Typical module structure

      Header            27
      Samples           up to 16*99=1584 (0x630)
      Positions         up to 513 (0x201)
      Patterns offsets  up to 32*7=224 (0xe0)
      0xff
      Patterns data     max offset = 2348 (0x92c) max size ~32pat*64lin*3chan*2byte=0x3000
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      uint16_t PositionsOffset;
      uint16_t OrnamentsOffset;
      uint16_t PatternsOffset;
      char Identifier[18];
      uint16_t Size;
    } PACK_POST;

    PACK_PRE struct RawPositions
    {
      uint8_t Length;
      PACK_PRE struct PosEntry
      {
        uint8_t PatternNum;
        int8_t Transposition;
      } PACK_POST;
      PosEntry Data[1];
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      uint8_t Number;
      boost::array<uint16_t, 3> Offsets;
    } PACK_POST;

    PACK_PRE struct RawOrnament
    {
      uint8_t Number;
      boost::array<int8_t, ORNAMENT_SIZE> Data;
    } PACK_POST;

    PACK_PRE struct RawSample
    {
      uint8_t Number;
      // EEEEaaaa
      // NESnnnnn
      // eeeeeeee
      // Ee - effect
      // a - level
      // N - noise mask, n- noise value
      // E - envelope mask
      // S - effect sign
      PACK_PRE struct Line
      {
        uint8_t EffHiAndLevel;
        uint8_t NoiseAndMasks;
        uint8_t EffLo;

        uint_t GetLevel() const
        {
          return EffHiAndLevel & 15;
        }

        int_t GetEffect() const
        {
          const int_t val = int_t(EffHiAndLevel & 240) * 16 + EffLo;
          return (NoiseAndMasks & 32) ? val : -val;
        }

        uint_t GetNoise() const
        {
          return NoiseAndMasks & 31;
        }

        bool GetNoiseMask() const
        {
          return 0 != (NoiseAndMasks & 128);
        }

        bool GetEnvelopeMask() const
        {
          return 0 != (NoiseAndMasks & 64);
        }
      } PACK_POST;
      Line Data[SAMPLE_SIZE];
      uint8_t Loop;
      uint8_t LoopSize;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 27);
    BOOST_STATIC_ASSERT(sizeof(RawPositions) == 3);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 7);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 33);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 99);

    bool IsProgramName(const String& name)
    {
      static const std::string STANDARD_PROGRAMS[] = 
      {
        "SONG BY ST COMPIL\x01",
        "SONG BY ST COMPILE",
        "SONG BY MB COMPILE",
        "SONG BY ST-COMPILE",
        "SONG ST BY COMPILE"
        "SOUND TRACKER v1.1",
        "SOUND TRACKER v1.3",
        "SOUND TRACKER v3.0",
        "S.T.FULL EDITION  ",
        "S.T.FULL EDITION \x7f",
        "S.W.COMPILE V2.0  ",
        "STU SONG COMPILER ",
      };
      return boost::end(STANDARD_PROGRAMS) != std::find(STANDARD_PROGRAMS, boost::end(STANDARD_PROGRAMS), ToStdString(name));
    }

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Limit(std::min(data.Size(), MAX_SIZE))
        , Delegate(data, Limit)
        , Source(*Delegate.GetField<RawHeader>(0))
        , TotalRanges(RangeChecker::CreateSimple(Limit))
        , FixedRanges(RangeChecker::CreateSimple(Limit))
      {
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        const String id(FromCharArray(Source.Identifier));
        if (IsProgramName(id))
        {
          meta.SetProgram(id);
        }
        else
        {
          meta.SetTitle(id);
          meta.SetProgram(Text::SOUNDTRACKER_DECODER_DESCRIPTION);
        }
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<PositionEntry> positions;
        for (RangeIterator<const RawPositions::PosEntry*> iter = GetPositions(); iter; ++iter)
        {
          const RawPositions::PosEntry& src = *iter;
          Require(Math::InRange<uint_t>(src.PatternNum, 1, MAX_PATTERNS_COUNT));
          PositionEntry dst;
          dst.PatternIndex = src.PatternNum - 1;
          dst.Transposition = src.Transposition;
          positions.push_back(dst);
        }
        builder.SetPositions(positions);
        Dbg("Positions: %1% entries", positions.size());
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Indices donePats(0, MAX_PATTERNS_COUNT - 1);
        for (uint_t patEntryIdx = 0; patEntryIdx < MAX_PATTERNS_COUNT; ++patEntryIdx)
        {
          const RawPattern& src = GetPattern(patEntryIdx);
          if (!Math::InRange<uint_t>(src.Number, 1, MAX_PATTERNS_COUNT))
          {
            break;
          }
          const uint_t patIndex = src.Number - 1;
          if (!pats.Contain(patIndex) || donePats.Contain(patIndex))
          {
            continue;
          }
          donePats.Insert(patIndex);
          Dbg("Parse pattern %1%", patIndex);
          ParsePattern(src, builder);
          if (pats.Count() == donePats.Count())
          {
            return;
          }
        }
        Require(!donePats.Empty());
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t idx = *it;
          if (!donePats.Contain(idx))
          {
            Dbg("Fill stub pattern %1%", idx);
            builder.StartPattern(idx);
          }
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Indices doneSams(0, MAX_SAMPLES_COUNT - 1);
        for (uint_t samEntryIdx = 0; samEntryIdx < MAX_SAMPLES_COUNT; ++samEntryIdx)
        {
          const RawSample& src = GetSample(samEntryIdx);
          const uint_t samIdx = src.Number;
          if (!samples.Contain(samIdx) || doneSams.Contain(samIdx))
          {
            continue;
          }
          doneSams.Insert(samIdx);
          Dbg("Parse sample %1%", samIdx);
          Sample result;
          ParseSample(src, result);
          builder.SetSample(samIdx, result);
          if (doneSams.Count() == samples.Count())
          {
            return;
          }
        }
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t idx = *it;
          if (!doneSams.Contain(idx))
          {
            Dbg("Fill stub sample %1%", idx);
            builder.SetSample(idx, Sample());
          }
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        if (ornaments.Empty())
        {
          Dbg("No ornaments used");
          return;
        }
        Indices doneOrns(0, MAX_ORNAMENTS_COUNT - 1);
        for (uint_t ornEntryIdx = 0; ornEntryIdx < MAX_ORNAMENTS_COUNT; ++ornEntryIdx)
        {
          const RawOrnament& src = GetOrnament(ornEntryIdx);
          const uint_t ornIdx = src.Number;
          if (!ornaments.Contain(ornIdx) || doneOrns.Contain(ornIdx))
          {
            continue;
          }
          doneOrns.Insert(ornIdx);
          Dbg("Parse ornament %1%", ornIdx);
          const Ornament result(src.Data.begin(), src.Data.end());
          builder.SetOrnament(ornIdx, result);
          if (doneOrns.Count() == ornaments.Count())
          {
            return;
          }
        }
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t idx = *it;
          if (!doneOrns.Contain(idx))
          {
            Dbg("Fill stub ornament %1%", idx);
            builder.SetOrnament(idx, Ornament());
          }
        }
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
      RangeIterator<const RawPositions::PosEntry*> GetPositions() const
      {
        const std::size_t offset = fromLE(Source.PositionsOffset);
        const RawPositions* const positions = Delegate.GetField<RawPositions>(offset);
        Require(positions != 0);
        const uint_t length = positions->Length + 1;
        AddRange(offset, sizeof(*positions) + (length - 1) * sizeof(RawPositions::PosEntry));
        const RawPositions::PosEntry* const firstEntry = positions->Data;
        const RawPositions::PosEntry* const lastEntry = firstEntry + length;
        return RangeIterator<const RawPositions::PosEntry*>(firstEntry, lastEntry);
      }

      const RawPattern& GetPattern(uint_t index) const
      {
        return GetObject<RawPattern>(index, fromLE(Source.PatternsOffset));
      }

      const RawSample& GetSample(uint_t index) const
      {
        return GetObject<RawSample>(index, sizeof(Source));
      }

      const RawOrnament& GetOrnament(uint_t index) const
      {
        return GetObject<RawOrnament>(index, fromLE(Source.OrnamentsOffset));
      }

      template<class T>
      const T& GetObject(uint_t index, std::size_t baseOffset) const
      {
        const std::size_t offset = baseOffset + index * sizeof(T);
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != 0);
        AddRange(offset, sizeof(T));
        return *src;
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const uint8_t* const data = Delegate.GetField<uint8_t>(offset);
        Require(data != 0);
        return *data;
      }

      struct DataCursors : public boost::array<std::size_t, 3>
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
          uint_t Period;
          uint_t Counter;

          ChannelState()
            : Offset()
            , Period()
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

        boost::array<ChannelState, 3> Channels;

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

      void ParsePattern(const RawPattern& src, Builder& builder) const
      {
        PatternBuilder& patBuilder = builder.StartPattern(src.Number - 1);
        const DataCursors rangesStarts(src);

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
          ParseLine(state, builder);
        }
        for (uint_t chanNum = 0; chanNum != rangesStarts.size(); ++chanNum)
        {
          const std::size_t start = rangesStarts[chanNum];
          if (start >= Limit)
          {
            Dbg("Invalid offset (%1%)", start);
          }
          else
          {
            const std::size_t stop = std::min(Limit, state.Channels[chanNum].Offset + 1);
            Dbg("Affected ranges %1%..%2%", start, stop);
            AddFixedRange(start, stop - start);
          }
        }
      }

      bool HasLine(const ParserState& src) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
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
          else if (0 == chan && 0xff == PeekByte(state.Offset))
          {
            return false;
          }
        }
        return true;
      }

      void ParseLine(ParserState& src, Builder& builder) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter--)
          {
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(state, builder);
          state.Counter = state.Period;
        }
      }

      void ParseChannel(ParserState::ChannelState& state, Builder& builder) const
      {
        while (state.Offset < Limit)
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd <= 0x5f)//note
          {
            builder.SetNote(cmd);
            break;
          }
          else if (cmd <= 0x6f)//sample
          {
            builder.SetSample(cmd - 0x60);
          }
          else if (cmd <= 0x7f)//ornament
          {
            builder.SetNoEnvelope();
            builder.SetOrnament(cmd - 0x70);
          }
          else if (cmd == 0x80)//reset
          {
            builder.SetRest();
            break;
          }
          else if (cmd == 0x81)//empty
          {
            break;
          }
          else if (cmd == 0x82) //orn 0 without envelope
          {
            builder.SetOrnament(0);
            builder.SetNoEnvelope();
          }
          else if (cmd <= 0x8e)//orn 0, with envelope
          {
            builder.SetOrnament(0);
            const uint_t envPeriod = PeekByte(state.Offset++);
            builder.SetEnvelope(cmd - 0x80, envPeriod);
          }
          else
          {
            state.Period = (cmd - 0xa1) & 0xff;
          }
        }
      }

      static void ParseSample(const RawSample& src, Sample& dst)
      {
        dst.Lines.resize(SAMPLE_SIZE);
        for (uint_t idx = 0; idx < SAMPLE_SIZE; ++idx)
        {
          const RawSample::Line& line = src.Data[idx];
          Sample::Line& res = dst.Lines[idx];
          res.Level = line.GetLevel();
          res.Noise = line.GetNoise();
          res.NoiseMask = line.GetNoiseMask();
          res.EnvelopeMask = line.GetEnvelopeMask();
          res.Effect = line.GetEffect();
        }
        dst.Loop = std::min<uint_t>(src.Loop, SAMPLE_SIZE);
        dst.LoopLimit = std::min<uint_t>(src.Loop + src.LoopSize + 1, SAMPLE_SIZE);
      }

      void AddRange(std::size_t offset, std::size_t size) const
      {
        Require(TotalRanges->AddRange(offset, size));
      }

      void AddFixedRange(std::size_t offset, std::size_t size) const
      {
        AddRange(offset, size);
        Require(FixedRanges->AddRange(offset, size));
      }
    private:
      const std::size_t Limit;
      const Binary::TypedContainer Delegate;
      const RawHeader& Source;
      const RangeChecker::Ptr TotalRanges;
      const RangeChecker::Ptr FixedRanges;
    };

    enum AreaTypes
    {
      HEADER,
      SAMPLES,
      POSITIONS,
      ORNAMENTS,
      PATTERNS,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        AddArea(SAMPLES, sizeof(header));
        AddArea(POSITIONS, fromLE(header.PositionsOffset));
        //first ornament can be overlapped by the other structures
        AddArea(ORNAMENTS, fromLE(header.OrnamentsOffset));
        AddArea(PATTERNS, fromLE(header.PatternsOffset));
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return sizeof(RawHeader) == GetAreaSize(HEADER) && Undefined == GetAreaSize(END);
      }

      bool CheckPositions(uint_t positions) const
      {
        const std::size_t size = GetAreaSize(POSITIONS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawPositions) + positions * sizeof(RawPositions::PosEntry);
        if (0 != size && size == requiredSize)
        {
          return true;
        }
        return IsOverlapFirstOrnament(POSITIONS, requiredSize);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawSample);
        if (0 != size && 0 == size % requiredSize)
        {
          return true;
        }
        return IsOverlapFirstOrnament(SAMPLES, requiredSize);
      }

      bool CheckOrnaments() const
      {
        const std::size_t size = GetAreaSize(ORNAMENTS);
        return 0 != size && Undefined != size && 0 == size % sizeof(RawOrnament);
      }
    private:
      bool IsOverlapFirstOrnament(AreaTypes type, std::size_t minSize) const
      {
        const std::size_t start = GetAreaAddress(type);
        const std::size_t orn = GetAreaAddress(ORNAMENTS);
        return start < orn && start + minSize > orn && start + minSize < orn + sizeof(RawOrnament);
      }
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size = std::min(rawData.Size(), MAX_SIZE);
      const Binary::TypedContainer data(rawData, size);
      const RawHeader* const hdr = data.GetField<RawHeader>(0);
      if (0 == hdr)
      {
        return false;
      }
      const Areas areas(*hdr, size);
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
      if (const RawPositions* positions = data.GetField<RawPositions>(areas.GetAreaAddress(POSITIONS)))
      {
        if (!areas.CheckPositions(positions->Length))
        {
          return false;
        }
      }
      else
      {
        return false;
      }
      if (0 == data.GetField<RawOrnament>(areas.GetAreaAddress(ORNAMENTS)))
      {
        return false;
      }
      if (0 == data.GetField<RawPattern>(areas.GetAreaAddress(PATTERNS)))
      {
        return false;
      }
      return true;
    }

    //Statistic-based format based on 6k+ files
    const std::string FORMAT(
    "01-20"       // uint8_t Tempo; 1..50
    "?00-07"      // uint16_t PositionsOffset;
    "?00-07"      // uint16_t OrnamentsOffset;
    "?00-08"      // uint16_t PatternsOffset;
    "?{20}"       // Id+Size
    "00-0f"       // first sample index
    );

    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& rawData, Builder& target)
    {
      if (!FastCheck(rawData))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        const Format format(rawData);

        format.ParseCommonProperties(target);

        StatisticCollectingBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        Require(statistic.HasNonEmptyPatterns());
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, statistic);
        Require(statistic.HasNonEmptySamples());
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

    class Decoder : public Formats::Chiptune::SoundTracker::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SOUNDTRACKERCOMPILED_DECODER_DESCRIPTION;
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
        return ParseCompiled(rawData, stub);
      }

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const
      {
        return ParseCompiled(data, target);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//SoundTrackerCompiled

  namespace SoundTracker
  {
    namespace Ver1
    {
      Decoder::Ptr CreateCompiledDecoder()
      {
        return boost::make_shared<SoundTrackerCompiled::Decoder>();
      }
    }
  }// namespace SoundTracker

  Formats::Chiptune::Decoder::Ptr CreateSoundTrackerCompiledDecoder()
  {
    return SoundTracker::Ver1::CreateCompiledDecoder();
  }
}// namespace Chiptune
}// namespace Formats
