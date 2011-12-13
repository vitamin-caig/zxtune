/*
Abstract:
  SoundTracker compiled format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtracker.h"
#include "soundtracker_detail.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <iterator.h>
#include <logging.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const std::string THIS_MODULE("Formats::Chiptune::SoundTrackerCompiled");
}

namespace Formats
{
namespace Chiptune
{
  namespace SoundTrackerCompiled
  {
    using namespace SoundTracker;

    const std::size_t MAX_MODULE_SIZE = 0x2500;

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

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Limit(std::min(data.Size(), MAX_MODULE_SIZE))
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
        builder.SetProgram(FromCharArray(Source.Identifier));
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<PositionEntry> positions;
        for (RangeIterator<const RawPositions::PosEntry*> iter = GetPositions(); iter; ++iter)
        {
          const RawPositions::PosEntry& src = *iter;
          Require(in_range<uint_t>(src.PatternNum, 1, MAX_PATTERNS_COUNT));
          PositionEntry dst;
          dst.PatternIndex = src.PatternNum - 1;
          dst.Transposition = src.Transposition;
          positions.push_back(dst);
        }
        builder.SetPositions(positions);
        Log::Debug(THIS_MODULE, "Positions: %1% entries", positions.size());
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Require(!pats.empty());
        Indices restPats(pats);
        for (uint_t patEntryIdx = 0; patEntryIdx < MAX_PATTERNS_COUNT && !restPats.empty(); ++patEntryIdx)
        {
          const RawPattern& src = GetPattern(patEntryIdx);
          const uint_t patIndex = src.Number - 1;
          if (restPats.count(patIndex))
          {
            Log::Debug(THIS_MODULE, "Parse pattern %1%", patIndex);
            builder.StartPattern(patIndex);
            ParsePattern(src, builder);
            restPats.erase(patIndex);
          }
        }
        Require(restPats.size() != pats.size());
        while (!restPats.empty())
        {
          const uint_t idx = *restPats.begin();
          Log::Debug(THIS_MODULE, "Fill stub pattern %1%", idx);
          builder.StartPattern(idx);
          restPats.erase(idx);
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Require(!samples.empty());
        Indices restSams(samples);
        for (uint_t samEntryIdx = 0; samEntryIdx < MAX_SAMPLES_COUNT && !restSams.empty(); ++samEntryIdx)
        {
          const RawSample& src = GetSample(samEntryIdx);
          const uint_t samIdx = src.Number;
          if (!restSams.count(samIdx))
          {
            continue;
          }
          Require(in_range<uint_t>(samIdx + 1, 1, MAX_SAMPLES_COUNT));
          Log::Debug(THIS_MODULE, "Parse sample %1%", samIdx);
          Sample result;
          ParseSample(src, result);
          builder.SetSample(samIdx, result);
          restSams.erase(samIdx);
        }
        while (!restSams.empty())
        {
          const uint_t idx = *restSams.begin();
          Log::Debug(THIS_MODULE, "Fill stub sample %1%", idx);
          builder.SetSample(idx, Sample());
          restSams.erase(idx);
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        if (ornaments.empty())
        {
          Log::Debug(THIS_MODULE, "No ornaments used");
          return;
        }
        Indices restOrns(ornaments);
        for (uint_t ornEntryIdx = 0; ornEntryIdx < MAX_ORNAMENTS_COUNT && !restOrns.empty(); ++ornEntryIdx)
        {
          const RawOrnament& src = GetOrnament(ornEntryIdx);
          const uint_t ornIdx = src.Number;
          if (!restOrns.count(ornIdx))
          {
            continue;
          }
          Require(in_range<uint_t>(ornIdx + 1, 1, MAX_ORNAMENTS_COUNT));
          Log::Debug(THIS_MODULE, "Parse ornament %1%", ornIdx);
          const Ornament result(src.Data.begin(), src.Data.end());
          builder.SetOrnament(ornIdx, result);
          restOrns.erase(ornIdx);
        }
        while (!restOrns.empty())
        {
          const uint_t idx = *restOrns.begin();
          Log::Debug(THIS_MODULE, "Fill stub ornament %1%", idx);
          builder.SetOrnament(idx, Ornament());
          restOrns.erase(idx);
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
        DataCursors Offsets;
        boost::array<uint_t, 3> Periods;
        boost::array<uint_t, 3> Counters;

        explicit ParserState(const DataCursors& src)
          : Offsets(src)
          , Periods()
          , Counters()
        {
        }

        uint_t GetMinCounter() const
        {
          return *std::min_element(Counters.begin(), Counters.end());
        }

        void SkipLines(uint_t toSkip)
        {
          std::transform(Counters.begin(), Counters.end(), Counters.begin(), std::bind2nd(std::minus<uint_t>(), toSkip));
        }
      };

      void ParsePattern(const RawPattern& src, Builder& builder) const
      {
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
            break;
          }
          builder.StartLine(lineIdx);
          ParseLine(state, builder);
        }
        builder.FinishPattern(lineIdx);
        for (uint_t chanNum = 0; chanNum != rangesStarts.size(); ++chanNum)
        {
          const std::size_t start = rangesStarts[chanNum];
          //TODO: improve size detection
          const std::size_t stop = std::min(Limit, state.Offsets[chanNum] + 1);
          Log::Debug(THIS_MODULE, "Affected ranges %1%..%2%", start, stop);
          AddFixedRange(start, stop - start);
        }
      }

      bool HasLine(ParserState& src) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          if (src.Counters[chan])
          {
            continue;
          }
          std::size_t& offset = src.Offsets[chan];
          if (offset >= Limit)
          {
            return false;
          }
          else if (0 == chan && 0xff == PeekByte(offset))
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
          uint_t& counter = src.Counters[chan];
          if (counter--)
          {
            continue;
          }
          std::size_t& offset = src.Offsets[chan];
          uint_t& period = src.Periods[chan];
          builder.StartChannel(chan);
          ParseChannel(offset, period, builder);
          counter = period;
        }
      }

      void ParseChannel(std::size_t& offset, uint_t& period, Builder& builder) const
      {
        while (offset < Limit)
        {
          const uint_t cmd = PeekByte(offset++);
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
            const uint_t envPeriod = PeekByte(offset++);
            builder.SetEnvelope(cmd - 0x80, envPeriod);
          }
          else
          {
            period = (cmd - 0xa1) & 0xff;
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

    class Container : public Formats::Chiptune::Container
    {
    public:
      Container(const Binary::Container& rawData, const Format& format)
        : Delegate(rawData.GetSubcontainer(0, format.GetSize()))
        , FixedArea(format.GetFixedArea())
      {
      }

      virtual std::size_t Size() const
      {
        return Delegate->Size();
      }

      virtual const void* Data() const
      {
        return Delegate->Data();
      }

      virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
      {
        return Delegate->GetSubcontainer(offset, size);
      }

      virtual uint_t FixedChecksum() const
      {
        return Crc32(safe_ptr_cast<const uint8_t*>(Delegate->Data()) + FixedArea.first, FixedArea.second - FixedArea.first);
      }
    private:
      const Binary::Container::Ptr Delegate;
      const RangeChecker::Range FixedArea;
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

    struct Areas : public AreaController<AreaTypes, 1 + END, std::size_t>
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
      const std::size_t size = std::min(rawData.Size(), MAX_MODULE_SIZE);
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

    const std::string FORMAT(
    "01-0f"       // uint8_t Tempo; 1..15
    "?00-09"      // uint16_t PositionsOffset;
    "?00-09"      // uint16_t OrnamentsOffset;
    "?00-09"      // uint16_t PatternsOffset;
    "+20+"        // Id+Size
    "00-0f"       // first sample index
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT))
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
        return FastCheck(rawData) && Format->Match(rawData.Data(), rawData.Size());
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//SoundTrackerCompiled

  namespace SoundTracker
  {
    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target)
    {
      using namespace SoundTrackerCompiled;

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

        return boost::make_shared<SoundTrackerCompiled::Container>(data, format);
      }
      catch (const std::exception&)
      {
        Log::Debug(THIS_MODULE, "Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }
  }// namespace SoundTracker

  Decoder::Ptr CreateSoundTrackerCompiledDecoder()
  {
    return boost::make_shared<SoundTrackerCompiled::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats
