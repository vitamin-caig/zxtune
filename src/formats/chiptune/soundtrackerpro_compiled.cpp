/*
Abstract:
  SoundTrackerPro compiled format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "metainfo.h"
#include "soundtrackerpro.h"
#include "soundtrackerpro_detail.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <iterator.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::SoundTrackerProCompiled");
}

namespace Formats
{
namespace Chiptune
{
  namespace SoundTrackerProCompiled
  {
    using namespace SoundTrackerPro;

    //size and offsets are taken from ~3400 modules analyzing
    const std::size_t MIN_SIZE = 200;
    const std::size_t MAX_SIZE = 0x2800;

    /*
      Typical module structure. Order is not changed according to different tests

      Header
      Id (optional)
      PatternsData
      Ornaments
      Samples
      Positions
      <possible gap due to invalid length>
      PatternsOffsets
      OrnamentsOffsets
      SamplesOffsets (may be trunkated)
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      uint16_t PositionsOffset;
      uint16_t PatternsOffset;
      uint16_t OrnamentsOffset;
      uint16_t SamplesOffset;
      uint8_t FixesCount;
    } PACK_POST;

    const uint8_t ID[] =
    {
      'K', 'S', 'A', ' ', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', ' ',
      'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
    };

    PACK_PRE struct RawId
    {
      uint8_t Id[sizeof(ID)];
      char Title[25];

      bool Check() const
      {
        return 0 == std::memcmp(Id, ID, sizeof(Id));
      }
    } PACK_POST;

    PACK_PRE struct RawPositions
    {
      uint8_t Length;
      uint8_t Loop;
      PACK_PRE struct PosEntry
      {
        uint8_t PatternOffset;//*6
        int8_t Transposition;
      } PACK_POST;
      PosEntry Data[1];
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      boost::array<uint16_t, 3> Offsets;
    } PACK_POST;

    PACK_PRE struct RawObject
    {
      int8_t Loop;
      int8_t Size;

      uint_t GetSize() const
      {
        return Size < 0
          ? 0
          : Size;
      }
    } PACK_POST;

    PACK_PRE struct RawOrnament : RawObject
    {
      typedef int8_t Line;

      Line GetLine(uint_t idx) const
      {
        const int8_t* const src = safe_ptr_cast<const int8_t*>(this + 1);
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        return src[offset];
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }
    } PACK_POST;

    PACK_PRE struct RawOrnaments
    {
      boost::array<uint16_t, MAX_ORNAMENTS_COUNT> Offsets;
    } PACK_POST;

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        // NxxTaaaa
        // xxnnnnnE
        // vvvvvvvv
        // vvvvvvvv

        // a - level
        // T - tone mask
        // N - noise mask
        // E - env mask
        // n - noise
        // v - signed vibrato
        uint8_t LevelAndFlags;
        uint8_t NoiseAndFlag;
        int16_t Vibrato;

        uint_t GetLevel() const
        {
          return LevelAndFlags & 15;
        }

        bool GetToneMask() const
        {
          return 0 != (LevelAndFlags & 16);
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelAndFlags & 128);
        }

        bool GetEnvelopeMask() const
        {
          return 0 != (NoiseAndFlag & 1);
        }

        uint_t GetNoise() const
        {
          return (NoiseAndFlag & 62) >> 1;
        }

        int_t GetVibrato() const
        {
          return fromLE(Vibrato);
        }
      } PACK_POST;

      Line GetLine(uint_t idx) const
      {
        BOOST_STATIC_ASSERT(0 == (sizeof(Line) & (sizeof(Line) - 1)));
        const uint_t maxLines = 256 / sizeof(Line);
        const Line* const src = safe_ptr_cast<const Line*>(this + 1);
        return src[idx % maxLines];
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }
    } PACK_POST;

    PACK_PRE struct RawSamples
    {
      boost::array<uint16_t, MAX_SAMPLES_COUNT> Offsets;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 10);
    BOOST_STATIC_ASSERT(sizeof(RawId) == 53);
    BOOST_STATIC_ASSERT(sizeof(RawPositions) == 4);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 6);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawOrnaments) == 32);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawSamples) == 30);

    class StubBuilder : public Builder
    {
    public:
      virtual void SetProgram(const String& /*program*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, const Sample& /*sample*/) {}
      virtual void SetOrnament(uint_t /*index*/, const Ornament& /*ornament*/) {}
      virtual void SetPositions(const std::vector<PositionEntry>& /*positions*/, uint_t /*loop*/) {}
      virtual void StartPattern(uint_t /*index*/) {}
      virtual void FinishPattern(uint_t /*size*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
      virtual void SetNoEnvelope() {}
      virtual void SetGliss(uint_t /*target*/) {}
      virtual void SetVolume(uint_t /*vol*/) {}
    };

    uint_t GetUnfixDelta(const RawHeader& hdr, const RawId& id, const RawPattern& firstPattern)
    {
      //first pattern is always placed after the header (and optional id);
      const std::size_t hdrSize = sizeof(hdr) + (id.Check() ? sizeof(id) : 0);
      const std::size_t firstData = fromLE(firstPattern.Offsets[0]);
      if (0 != hdr.FixesCount)
      {
        Require(firstData == hdrSize);
      }
      else
      {
        Require(firstData >= hdrSize);
      }
      return static_cast<uint_t>(firstData - hdrSize);
    }

    std::size_t GetMaxSize(const RawHeader& hdr)
    {
      return fromLE(hdr.SamplesOffset) + sizeof(RawSamples);
    }

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

    class Format
    {
    public:
      explicit Format(const Binary::TypedContainer& data)
        : Delegate(data)
        , Ranges(Delegate.GetSize())
        , Source(GetServiceObject<RawHeader>(0, 0))
        , Id(GetObject<RawId>(sizeof(Source)))
        , UnfixDelta(GetUnfixDelta(Source, Id, GetPattern(0)))
      {
        if (Id.Check())
        {
          Ranges.AddService(sizeof(Source), sizeof(Id));
        }
        if (UnfixDelta)
        {
          Dbg("Unfix delta is %1%", UnfixDelta);
        }
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        builder.SetProgram(Text::SOUNDTRACKERPRO_DECODER_DESCRIPTION);
        if (Id.Check())
        {
          builder.SetTitle(FromCharArray(Id.Title));
        }
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<PositionEntry> positions;
        for (RangeIterator<const RawPositions::PosEntry*> iter = GetPositions(); iter; ++iter)
        {
          const RawPositions::PosEntry& src = *iter;
          Require(0 == src.PatternOffset % 6);
          const uint_t patNum = src.PatternOffset / 6;
          Require(Math::InRange<uint_t>(patNum + 1, 1, MAX_PATTERNS_COUNT));
          PositionEntry dst;
          dst.PatternIndex = patNum;
          dst.Transposition = src.Transposition;
          positions.push_back(dst);
        }
        const uint_t loop = PeekByte(fromLE(Source.PositionsOffset) + offsetof(RawPositions, Loop));
        builder.SetPositions(positions, loop);
        Dbg("Positions: %1% entries, loop to %2%", positions.size(), loop);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Require(!pats.empty());
        Dbg("Patterns: %1% to parse", pats.size());
        bool hasValidPatterns = false;
        const uint_t minPatternsOffset = sizeof(Source) + (Id.Check() ? sizeof(Id) : 0);
        for (Indices::const_iterator it = pats.begin(), lim = pats.end(); it != lim; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          const RawPattern& src = GetPattern(patIndex);
          builder.StartPattern(patIndex);
          if (ParsePattern(src, minPatternsOffset, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Require(!samples.empty());
        Dbg("Samples: %1% to parse", samples.size());
        for (Indices::const_iterator it = samples.begin(), lim = samples.end(); it != lim; ++it)
        {
          const uint_t samIdx = *it;
          Require(Math::InRange<uint_t>(samIdx, 0, MAX_SAMPLES_COUNT - 1));
          Dbg("Parse sample %1%", samIdx);
          const RawSample& src = GetSample(samIdx);
          Sample result;
          ParseSample(src, result);
          builder.SetSample(samIdx, result);
        }
        //mark possible samples offsets as used
        const std::size_t samplesOffsets = fromLE(Source.SamplesOffset);
        Ranges.Add(samplesOffsets, std::min(sizeof(RawSamples), Delegate.GetSize() - samplesOffsets));
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Require(!ornaments.empty() && 0 == *ornaments.begin());
        Dbg("Ornaments: %1% to parse", ornaments.size());
        for (Indices::const_iterator it = ornaments.begin(), lim = ornaments.end(); it != lim; ++it)
        {
          const uint_t ornIdx = *it;
          Require(Math::InRange<uint_t>(ornIdx, 0, MAX_ORNAMENTS_COUNT - 1));
          Dbg("Parse ornament %1%", ornIdx);
          const RawOrnament& src = GetOrnament(ornIdx);
          Ornament result;
          ParseOrnament(src, result);
          builder.SetOrnament(ornIdx, result);
        }
      }

      std::size_t GetSize() const
      {
        const std::size_t result = Ranges.GetSize();
        Require(result + UnfixDelta <= 0x10000);
        return result;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return Ranges.GetFixedArea();
      }
    private:
      RangeIterator<const RawPositions::PosEntry*> GetPositions() const
      {
        const std::size_t offset = fromLE(Source.PositionsOffset);
        const RawPositions* const positions = Delegate.GetField<RawPositions>(offset);
        Require(positions != 0);
        const uint_t length = positions->Length;
        Require(length != 0);
        Ranges.AddService(offset, sizeof(*positions) + (length - 1) * sizeof(RawPositions::PosEntry));
        const RawPositions::PosEntry* const firstEntry = positions->Data;
        const RawPositions::PosEntry* const lastEntry = firstEntry + length;
        return RangeIterator<const RawPositions::PosEntry*>(firstEntry, lastEntry);
      }

      const RawPattern& GetPattern(uint_t index) const
      {
        return GetServiceObject<RawPattern>(index, fromLE(Source.PatternsOffset));
      }

      const RawSample& GetSample(uint_t index) const
      {
        const uint16_t offset = fromLE(GetServiceObject<uint16_t>(index, fromLE(Source.SamplesOffset))) - UnfixDelta;
        const RawObject* const obj = Delegate.GetField<RawObject>(offset);
        Require(obj != 0);
        const RawSample* const res = safe_ptr_cast<const RawSample*>(obj);
        Ranges.Add(offset, res->GetUsedSize());
        return *res;
      }

      const RawOrnament& GetOrnament(uint_t index) const
      {
        const uint16_t offset = fromLE(GetServiceObject<uint16_t>(index, fromLE(Source.OrnamentsOffset))) - UnfixDelta;
        const RawObject* const obj = Delegate.GetField<RawObject>(offset);
        Require(obj != 0);
        const RawOrnament* const res = safe_ptr_cast<const RawOrnament*>(obj);
        Ranges.Add(offset, res->GetUsedSize());
        return *res;
      }

      template<class T>
      const T& GetObject(uint_t offset) const
      {
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != 0);
        Ranges.Add(offset, sizeof(T));
        return *src;
      }

      template<class T>
      const T& GetServiceObject(std::size_t index, std::size_t baseOffset) const
      {
        const std::size_t offset = baseOffset + index * sizeof(T);
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != 0);
        Ranges.AddService(offset, sizeof(T));
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
        DataCursors(const RawPattern& src, uint_t minOffset, uint_t unfixDelta)
        {
          std::transform(src.Offsets.begin(), src.Offsets.end(), begin(), std::ptr_fun(&fromLE<uint16_t>));
          Require(end() == std::find_if(begin(), end(), std::bind2nd(std::less<uint_t>(), minOffset + unfixDelta)));
          std::transform(begin(), end(), begin(), std::bind2nd(std::minus<uint_t>(), unfixDelta));
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

      bool ParsePattern(const RawPattern& src, uint_t minOffset, Builder& builder) const
      {
        const DataCursors rangesStarts(src, minOffset, UnfixDelta);
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
            builder.FinishPattern(std::max<uint_t>(lineIdx, MIN_PATTERN_SIZE));
            break;
          }
          builder.StartLine(lineIdx);
          ParseLine(state, builder);
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
            const std::size_t stop = std::min(Delegate.GetSize(), state.Offsets[chanNum] + 1);
            Ranges.AddFixed(start, stop - start);
          }
        }
        return lineIdx >= MIN_PATTERN_SIZE;
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
          if (offset >= Delegate.GetSize())
          {
            return false;
          }
          else if (0 == chan && 0x00 == PeekByte(offset))
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
        while (offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(offset++);
          if (cmd == 0)
          {
            continue;
          }
          else if (cmd <= 0x60)//note
          {
            builder.SetNote(cmd - 1);
            break;
          }
          else if (cmd <= 0x6f)//sample
          {
            builder.SetSample(cmd - 0x61);
          }
          else if (cmd <= 0x7f)//ornament
          {
            builder.SetOrnament(cmd - 0x70);
            builder.SetNoEnvelope();
            builder.SetGliss(0);
          }
          else if (cmd <= 0xbf) //skip
          {
            period = cmd - 0x80;
          }
          else if (cmd <= 0xcf) //envelope
          {
            if (cmd != 0xc0)
            {
              builder.SetEnvelope(cmd - 0xc0, PeekByte(offset++));
            }
            else
            {
              builder.SetEnvelope(0, 0);
            }
            builder.SetOrnament(0);
            builder.SetGliss(0);
          }
          else if (cmd <= 0xdf) //reset
          {
            builder.SetRest();
            break;
          }
          else if (cmd <= 0xef)//empty
          {
            break;
          }
          else if (cmd == 0xf0)//glissade
          {
            builder.SetGliss(static_cast<int8_t>(PeekByte(offset++)));
          }
          else //volume
          {
            builder.SetVolume(cmd - 0xf1);
          }
        }
      }

      static void ParseSample(const RawSample& src, Sample& dst)
      {
        const uint_t size = src.GetSize();
        dst.Lines.resize(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          Sample::Line& res = dst.Lines[idx];
          res.Level = line.GetLevel();
          res.Noise = line.GetNoise();
          res.ToneMask = line.GetToneMask();
          res.NoiseMask = line.GetNoiseMask();
          res.EnvelopeMask = line.GetEnvelopeMask();
          res.Vibrato = line.GetVibrato();
        }
        dst.Loop = std::min<int_t>(src.Loop, size);
      }

      static void ParseOrnament(const RawOrnament& src, Ornament& dst)
      {
        const uint_t size = src.GetSize();
        dst.Lines.resize(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          dst.Lines[idx] = src.GetLine(idx);
        }
        dst.Loop = std::min<int_t>(src.Loop, size);
      }
    private:
      const Binary::TypedContainer Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
      const RawId& Id;
      const uint_t UnfixDelta;
    };

    enum AreaTypes
    {
      HEADER,
      IDENTIFIER,
      POSITIONS,
      PATTERNS,
      ORNAMENTS,
      SAMPLES,
      END
    };

    struct Areas : public AreaController<AreaTypes, 1 + END, std::size_t>
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        const std::size_t idOffset = sizeof(header);
        if (idOffset + sizeof(RawId) <= size)
        {
          const RawId* const id = safe_ptr_cast<const RawId*>(&header + 1);
          if (id->Check())
          {
            AddArea(IDENTIFIER, sizeof(header));
          }
        }
        AddArea(POSITIONS, fromLE(header.PositionsOffset));
        AddArea(PATTERNS, fromLE(header.PatternsOffset));
        AddArea(ORNAMENTS, fromLE(header.OrnamentsOffset));
        AddArea(SAMPLES, fromLE(header.SamplesOffset));
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return sizeof(RawHeader) <= GetAreaSize(HEADER) && Undefined == GetAreaSize(END);
      }

      bool CheckPositions(uint_t positions) const
      {
        if (!positions)
        {
          return false;
        }
        const std::size_t size = GetAreaSize(POSITIONS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawPositions) + (positions - 1) * sizeof(RawPositions::PosEntry);
        if (requiredSize > size)
        {
          //no place
          return false;
        }
        //check for possible invalid length in header- real positions should fit place
        return 0 == (size - requiredSize) % sizeof(RawPositions::PosEntry);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES);
        if (Undefined == size)
        {
          return false;
        }
        //samples offsets should be the last region
        return IsLast(SAMPLES);
      }

      bool CheckOrnaments() const
      {
        const std::size_t size = GetAreaSize(ORNAMENTS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawOrnaments);
        return requiredSize == size;
      }
    private:
      bool IsLast(AreaTypes area) const
      {
        const std::size_t addr = GetAreaAddress(area);
        const std::size_t size = GetAreaSize(area);
        const std::size_t limit = GetAreaAddress(END);
        return addr + size == limit;
      }
    };

    Binary::TypedContainer CreateContainer(const Binary::Container& rawData)
    {
      const std::size_t size = std::min(rawData.Size(), MAX_SIZE);
      return Binary::TypedContainer(rawData, size);
    }

    bool FastCheck(const Binary::TypedContainer& data)
    {
      const RawHeader* const hdr = data.GetField<RawHeader>(0);
      if (0 == hdr)
      {
        return false;
      }
      const Areas areas(*hdr, std::min(data.GetSize(), GetMaxSize(*hdr)));
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
      return true;
    }

    bool FastCheck(const Binary::Container& rawData)
    {
      const Binary::TypedContainer data(CreateContainer(rawData));
      return FastCheck(data);
    }

    const std::string FORMAT(
      "03-0f"  // uint8_t Tempo; 3..15
      "?00-26" // uint16_t PositionsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t PatternsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t OrnamentsOffset; 0..MAX_MODULE_SIZE
      "?00-27" // uint16_t SamplesOffset; 0..MAX_MODULE_SIZE
    );

    class Decoder : public Formats::Chiptune::SoundTrackerPro::Decoder
    {
    public:
      Decoder()
        : Header(Binary::Format::Create(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SOUNDTRACKERPROCOMPILED_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Header;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Header->Match(rawData) && FastCheck(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target) const
      {
        if (!Check(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        try
        {
          const Binary::TypedContainer totalData(rawData);
          const std::size_t maxSize = GetMaxSize(*totalData.GetField<RawHeader>(0));
          const Binary::TypedContainer data(rawData, std::min(rawData.Size(), maxSize));
          const Format format(data);

          format.ParseCommonProperties(target);

          StatisticCollectingBuilder statistic(target);
          format.ParsePositions(statistic);
          const Indices& usedPatterns = statistic.GetUsedPatterns();
          format.ParsePatterns(usedPatterns, statistic);
          const Indices& usedSamples = statistic.GetUsedSamples();
          format.ParseSamples(usedSamples, target);
          Indices usedOrnaments = statistic.GetUsedOrnaments();
          usedOrnaments.insert(0);
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

      virtual Binary::Container::Ptr InsertMetainformation(const Binary::Container& rawData, const Dump& info) const
      {
        StatisticCollectingBuilder statistic(GetStubBuilder());
        if (Binary::Container::Ptr parsed = Parse(rawData, statistic))
        {
          const Binary::TypedContainer typedHelper(CreateContainer(*parsed));
          const RawHeader& header = *typedHelper.GetField<RawHeader>(0);
          const std::size_t headerSize = sizeof(header);
          const std::size_t infoSize = info.size();
          const PatchedDataBuilder::Ptr patch = PatchedDataBuilder::Create(*parsed);
          const RawId* const id = typedHelper.GetField<RawId>(headerSize);
          if (id && id->Check())
          {
            patch->OverwriteData(headerSize, info);
          }
          else
          {
            patch->InsertData(headerSize, info);
            const int_t delta = static_cast<int_t>(infoSize);
            patch->FixLEWord(offsetof(RawHeader, PositionsOffset), delta);
            patch->FixLEWord(offsetof(RawHeader, PatternsOffset), delta);
            patch->FixLEWord(offsetof(RawHeader, OrnamentsOffset), delta);
            patch->FixLEWord(offsetof(RawHeader, SamplesOffset), delta);
            const std::size_t patternsStart = fromLE(header.PatternsOffset);
            Indices usedPatterns = statistic.GetUsedPatterns();
            //first pattern is used to detect fixdelta
            usedPatterns.insert(0);
            for (Indices::const_iterator it = usedPatterns.begin(), lim = usedPatterns.end(); it != lim; ++it)
            {
              const std::size_t patOffsets = patternsStart + *it * sizeof(RawPattern);
              patch->FixLEWord(patOffsets + 0, delta);
              patch->FixLEWord(patOffsets + 2, delta);
              patch->FixLEWord(patOffsets + 4, delta);
            }
            const std::size_t ornamentsStart = fromLE(header.OrnamentsOffset);
            Indices usedOrnaments = statistic.GetUsedOrnaments();
            //first ornament is mandatory
            usedOrnaments.insert(0);
            for (Indices::const_iterator it = usedOrnaments.begin(), lim = usedOrnaments.end(); it != lim; ++it)
            {
              const std::size_t ornOffset = ornamentsStart + *it * sizeof(uint16_t);
              patch->FixLEWord(ornOffset, delta);
            }
            const std::size_t samplesStart = fromLE(header.SamplesOffset);
            const Indices& usedSamples = statistic.GetUsedSamples();
            for (Indices::const_iterator it = usedSamples.begin(), lim = usedSamples.end(); it != lim; ++it)
            {
              const std::size_t samOffset = samplesStart + *it * sizeof(uint16_t);
              patch->FixLEWord(samOffset, delta);
            }
          }
          return patch->GetResult();
        }
        return Binary::Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Header;
    };
  }//SoundTrackerProCompiled

  namespace SoundTrackerPro
  {
    Builder& GetStubBuilder()
    {
      static SoundTrackerProCompiled::StubBuilder stub;
      return stub;
    }

    Decoder::Ptr CreateCompiledModulesDecoder()
    {
      return boost::make_shared<SoundTrackerProCompiled::Decoder>();
    }
  }// namespace SoundTrackerPro

  Formats::Chiptune::Decoder::Ptr CreateSoundTrackerProCompiledDecoder()
  {
    return SoundTrackerPro::CreateCompiledModulesDecoder();
  }
}// namespace Chiptune
}// namespace Formats
