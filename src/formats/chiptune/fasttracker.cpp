/*
Abstract:
  FastTracker format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "fasttracker.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//std includes
#include <cstring>
#include <set>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::FastTracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace FastTracker
  {
    const std::size_t MAX_MODULE_SIZE = 0x3a00;
    const std::size_t SAMPLES_COUNT = 32;
    const std::size_t MAX_SAMPLE_SIZE = 80;
    const std::size_t ORNAMENTS_COUNT = 33;
    const std::size_t MAX_ORNAMENT_SIZE = 80;
    const std::size_t MIN_PATTERN_SIZE = 1;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const uint_t TEMPO_MIN = 3;
    const uint_t TEMPO_MAX = 255;

    /*
    Typical module structure:

    Header      (212 bytes)
     Id
     other
     samples offsets/addresses
     ornaments offsets/addresses
    Positions   (up to 510 bytes)
    #ff
    #ffff <- second padding[0]
    #ffff
    #ffff <- second padding[1]
    #ffff
    Patterns offsets/addresses
    #ffff
    Patterns data
    Samples
    Ornaments
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    const uint8_t MODULE_ID[] = 
    {
      'M', 'o', 'd', 'u', 'l', 'e', ':', ' '
    };

    PACK_PRE struct RawId
    {
      uint8_t Identifier[8];//"Module: "
      char Title[42];
      uint8_t Semicolon;//";"
      char Editor[18];

      bool HasTitle() const
      {
        BOOST_STATIC_ASSERT(sizeof(MODULE_ID) == sizeof(Identifier));
        return 0 == std::memcmp(Identifier, MODULE_ID, sizeof(Identifier));
      }

      bool HasProgram() const
      {
        return Semicolon == ';';
      }
    } PACK_POST;

    PACK_PRE struct RawHeader
    {
      RawId Metainfo;
      uint8_t Tempo;
      uint8_t Loop;
      uint8_t Padding1[4];
      uint16_t PatternsOffset;
      uint8_t Padding2[5];
      boost::array<uint16_t, SAMPLES_COUNT> SamplesOffsets;
      boost::array<uint16_t, ORNAMENTS_COUNT> OrnamentsOffsets;
    } PACK_POST;

    PACK_PRE struct RawPosition
    {
      uint8_t PatternIndex;
      int8_t Transposition;
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      boost::array<uint16_t, 3> Offsets;
    } PACK_POST;

    PACK_PRE struct RawObject
    {
      uint8_t Size;
      uint8_t Loop;
      uint8_t LoopLimit;

      uint_t GetSize() const
      {
        return Size + 1;
      }

      uint_t GetLoop() const
      {
        return Loop;
      }

      uint_t GetLoopLimit() const
      {
        return LoopLimit + 1;
      }
    } PACK_POST;

    PACK_PRE struct RawOrnament : RawObject
    {
      PACK_PRE struct Line
      {
        //nNxooooo
        //OOOOOOOO
        //o - noise offset
        //O - note offset (signed)
        //N - keep note offset
        //n - keep noise offset
        uint8_t FlagsAndNoiseAddon;
        int8_t NoteAddon;

        uint_t GetNoiseAddon() const
        {
          return FlagsAndNoiseAddon & 31;
        }

        bool GetKeepNoiseAddon() const
        {
          return 0 != (FlagsAndNoiseAddon & 128);
        }

        int_t GetNoteAddon() const
        {
          return NoteAddon;
        }

        bool GetKeepNoteAddon() const
        {
          return 0 != (FlagsAndNoiseAddon & 64);
        }
      } PACK_POST;

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + GetSize() * sizeof(Line);
      }

      Line GetLine(uint_t idx) const
      {
        const uint8_t* const src = safe_ptr_cast<const uint8_t*>(this + 1);
        //using 8-bit offsets
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        Line res;
        res.FlagsAndNoiseAddon = src[offset++];
        res.NoteAddon = src[offset++];
        return res;
      }
    } PACK_POST;

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        //KMxnnnnn
        //K - keep noise offset
        //M - noise mask
        //n - noise offset
        uint8_t Noise;
        uint8_t ToneLo;
        //KMxxtttt
        //K - keep tone offset
        //M - tone mask
        uint8_t ToneHi;
        //KMvvaaaa
        //vv = 0x - no slide
        //     10 - +1
        //     11 - -1
        //a - level
        //K - keep envelope offset
        //M - envelope mask
        uint8_t Level;
        int8_t EnvelopeAddon;

        uint_t GetLevel() const
        {
          return Level & 15;
        }

        int_t GetVolSlide() const
        {
          return Level & 32 ? (Level & 16 ? -1 : 1) : 0;
        }

        uint_t GetNoise() const
        {
          return Noise & 31;
        }

        bool GetAccumulateNoise() const
        {
          return 0 != (Noise & 128);
        }

        bool GetNoiseMask() const
        {
          return 0 != (Noise & 64);
        }

        uint_t GetTone() const
        {
          return uint_t(ToneHi & 15) * 256 + ToneLo;
        }

        bool GetAccumulateTone() const
        {
          return 0 != (ToneHi & 128);
        }

        bool GetToneMask() const
        {
          return 0 != (ToneHi & 64);
        }

        int_t GetEnvelopeAddon() const
        {
          return EnvelopeAddon;
        }

        bool GetAccumulateEnvelope() const
        {
          return 0 != (Level & 128);
        }

        bool GetEnableEnvelope() const
        {
          return 0 != (Level & 64);
        }

        bool IsEmpty() const
        {
          return Noise == 0 && ToneLo == 0 && ToneHi == 0 && Level == 0 && EnvelopeAddon == 0;
        }
      } PACK_POST;

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }

      const Line& GetLine(uint_t idx) const
      {
        return Lines[idx];
      }

      Line Lines[1];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawId) == 69);
    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 212);
    BOOST_STATIC_ASSERT(sizeof(RawPosition) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 6);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 3);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament::Line) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 8);
    BOOST_STATIC_ASSERT(sizeof(RawSample::Line) == 5);

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
      virtual void StartPattern(uint_t /*index*/) {}
      virtual void FinishPattern(uint_t /*size*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void SetTempo(uint_t /*tempo*/) {}
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetVolume(uint_t /*vol*/) {}
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*tone*/) {}
      virtual void SetNoEnvelope() {}
      virtual void SetNoise(uint_t /*val*/) {}
      virtual void SetSlide(uint_t /*step*/) {}
      virtual void SetNoteSlide(uint_t /*step*/) {}
    };

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
        , UsedSamples(0, SAMPLES_COUNT - 1)
        , UsedOrnaments(0, ORNAMENTS_COUNT - 1)
      {
        UsedSamples.Insert(0);
        UsedOrnaments.Insert(0);
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
        assert(UsedSamples.Contain(index));
        return Delegate.SetSample(index, sample);
      }

      virtual void SetOrnament(uint_t index, const Ornament& ornament)
      {
        assert(UsedOrnaments.Contain(index));
        return Delegate.SetOrnament(index, ornament);
      }

      virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop)
      {
        Require(!positions.empty());
        UsedPatterns.Clear();
        for (std::vector<PositionEntry>::const_iterator it = positions.begin(), lim = positions.end(); it != lim; ++it)
        {
          UsedPatterns.Insert(it->PatternIndex);
        }
        return Delegate.SetPositions(positions, loop);
      }

      virtual void StartPattern(uint_t index)
      {
        assert(UsedPatterns.Contain(index));
        return Delegate.StartPattern(index);
      }

      virtual void FinishPattern(uint_t size)
      {
        return Delegate.FinishPattern(size);
      }

      virtual void StartLine(uint_t index)
      {
        return Delegate.StartLine(index);
      }

      virtual void SetTempo(uint_t tempo)
      {
        Require(Math::InRange(tempo, TEMPO_MIN, TEMPO_MAX));
        return Delegate.SetTempo(tempo);
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

      virtual void SetVolume(uint_t vol)
      {
        return Delegate.SetVolume(vol);
      }

      virtual void SetEnvelope(uint_t type, uint_t tone)
      {
        return Delegate.SetEnvelope(type, tone);
      }

      virtual void SetNoEnvelope()
      {
        return Delegate.SetNoEnvelope();
      }

      virtual void SetNoise(uint_t val)
      {
        return Delegate.SetNoise(val);
      }

      virtual void SetSlide(uint_t step)
      {
        return Delegate.SetSlide(step);
      }

      virtual void SetNoteSlide(uint_t step)
      {
        return Delegate.SetNoteSlide(step);
      }

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

      const Indices& GetUsedSamples() const
      {
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
        : ServiceRanges(RangeChecker::CreateSimple(limit))
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
      Format(const Binary::TypedContainer& data, std::size_t baseAddr)
        : Delegate(data)
        , Ranges(data.GetSize())
        , Source(*Delegate.GetField<RawHeader>(0))
        , BaseAddr(baseAddr)
      {
        Ranges.AddService(0, sizeof(Source));
        Dbg("Base addr is #%1$04x", BaseAddr);
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        const RawId& in = Source.Metainfo;
        MetaBuilder& meta = builder.GetMetaBuilder();
        if (in.HasProgram())
        {
          meta.SetProgram(FromCharArray(in.Editor));
        }
        else
        {
          meta.SetProgram(Text::FASTTRACKER_DECODER_DESCRIPTION);
        }
        if (in.HasTitle())
        {
          meta.SetTitle(FromCharArray(in.Title));
        }
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<PositionEntry> positions;
        const std::size_t positionsStart = sizeof(Source);
        for (uint_t posIdx = 0; ; ++posIdx)
        {
          const RawPosition& pos = GetServiceObject<RawPosition>(positionsStart + sizeof(RawPosition) * posIdx);
          const uint_t patIdx = pos.PatternIndex;
          if (patIdx == 0xff)
          {
            break;
          }
          PositionEntry res;
          res.PatternIndex = patIdx;
          res.Transposition = pos.Transposition;
          positions.push_back(res);
        }
        const uint_t loop = Source.Loop;
        builder.SetPositions(positions, loop);
        Dbg("Positions: %1% entries, loop to %2%", positions.size(), loop);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: %1% to parse", pats.Count());
        const std::size_t baseOffset = fromLE(Source.PatternsOffset);
        const std::size_t minOffset = baseOffset + pats.Maximum() * sizeof(RawPattern);
        bool hasValidPatterns = false;
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          const RawPattern& src = GetServiceObject<RawPattern>(baseOffset + patIndex * sizeof(RawPattern));
          builder.StartPattern(patIndex);
          if (ParsePattern(src, minOffset, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Dbg("Samples: %1% to parse", samples.Count());
        uint_t nonEmptySamplesCount = 0;
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const std::size_t samOffset = fromLE(Source.SamplesOffsets[samIdx]) - BaseAddr;
          const std::size_t availSize = Delegate.GetSize() - samOffset;
          const RawSample* const src = Delegate.GetField<RawSample>(samOffset);
          Require(src != 0);
          Require(src->GetLoopLimit() <= src->GetSize());
          Require(src->GetLoop() <= src->GetLoopLimit());
          const std::size_t usedSize = src->GetUsedSize();
          Require(usedSize <= availSize);
          Dbg("Parse sample %1%", samIdx);
          Ranges.AddService(samOffset, usedSize);
          Sample result;
          ParseSample(*src, src->GetLoopLimit(), result);
          builder.SetSample(samIdx, result);
          if (src->GetLoopLimit() > 1 || !src->GetLine(0).IsEmpty())
          {
            ++nonEmptySamplesCount;
          }
        }
        Require(nonEmptySamplesCount > 0);
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: %1% to parse", ornaments.Count());
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Ornament result;
          const std::size_t ornOffset = fromLE(Source.OrnamentsOffsets[ornIdx]) - BaseAddr;
          const std::size_t availSize = Delegate.GetSize() - ornOffset;
          const RawOrnament* const src = Delegate.GetField<RawOrnament>(ornOffset);
          Require(src != 0);
          Require(src->GetLoopLimit() <= src->GetSize());
          Require(src->GetLoop() <= src->GetLoopLimit());
          const std::size_t usedSize = src->GetUsedSize();
          Require(usedSize <= availSize);
          Dbg("Parse ornament %1%", ornIdx);
          Ranges.AddService(ornOffset, usedSize);
          ParseOrnament(*src, src->GetLoopLimit(), result);
          builder.SetOrnament(ornIdx, result);
        }
        for (uint_t ornIdx = ornaments.Maximum() + 1; ornIdx < ORNAMENTS_COUNT; ++ornIdx)
        {
          const std::size_t ornOffset = fromLE(Source.OrnamentsOffsets[ornIdx]) - BaseAddr;
          const std::size_t availSize = Delegate.GetSize() - ornOffset;
          if (const RawOrnament* const src = Delegate.GetField<RawOrnament>(ornOffset))
          {
            const std::size_t usedSize = src->GetUsedSize();
            if (usedSize <= availSize)
            {
              Dbg("Stub ornament %1%", ornIdx);
              Ranges.Add(ornOffset, usedSize);
            }
          }
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
      template<class T>
      const T& GetServiceObject(std::size_t offset) const
      {
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
        DataCursors(const RawPattern& src, std::size_t baseOffset)
        {
          for (std::size_t chan = 0; chan != src.Offsets.size(); ++chan)
          {
            const std::size_t addr = fromLE(src.Offsets[chan]);
            Require(addr >= baseOffset);
            at(chan) = addr - baseOffset;
          }
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

      bool ParsePattern(const RawPattern& pat, std::size_t minOffset, Builder& builder) const
      {
        const DataCursors rangesStarts(pat, BaseAddr);
        Require(rangesStarts.end() == std::find_if(rangesStarts.begin(), rangesStarts.end(), !boost::bind(&Math::InRange<std::size_t>, _1, minOffset, Delegate.GetSize() - 1)));

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
        while (offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(offset++);
          if (cmd <= 0x1f)
          {
            builder.SetSample(cmd);
          }
          else if (cmd <= 0x2f)
          {
            const uint_t vol = cmd - 0x20;
            builder.SetVolume(vol);
          }
          else if (cmd == 0x30)
          {
            builder.SetRest();
            period = 0;
            break;
          }
          else if (cmd <= 0x3e)
          {
            const uint_t type = cmd - 0x30;
            const uint_t tone = PeekByte(offset) | (uint_t(PeekByte(offset + 1)) << 8);
            builder.SetEnvelope(type, tone);
            offset += 2;
          }
          else if (cmd == 0x3f)
          {
            builder.SetNoEnvelope();
          }
          else if (cmd <= 0x5f)
          {
            period = cmd - 0x40;
            break;
          }
          else if (cmd <= 0xcb)
          {
            builder.SetNote(cmd - 0x60);
            period = 0;
            break;
          }
          else if (cmd <= 0xec)
          {
            builder.SetOrnament(cmd - 0xcc);
          }
          else if (cmd == 0xed)
          {
            const uint_t step = PeekByte(offset) | (uint_t(PeekByte(offset + 1)) << 8);
            builder.SetSlide(step);
            offset += 2;
          }
          else if (cmd == 0xee)
          {
            const uint_t noteSlideStep = PeekByte(offset++);
            builder.SetNoteSlide(noteSlideStep);
          }
          else if (cmd == 0xef)
          {
            const uint_t noise = PeekByte(offset++);
            builder.SetNoise(noise);
          }
          else
          {
            const uint_t tempo = PeekByte(offset++);
            builder.SetTempo(tempo);
          }
        }
      }

      static void ParseSample(const RawSample& src, uint_t size, Sample& dst)
      {
        Require(size <= MAX_SAMPLE_SIZE);
        dst.Lines.resize(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          dst.Lines[idx] = ParseSampleLine(line);
        }
        dst.Loop = std::min<uint_t>(src.GetLoop(), dst.Lines.size());
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& src)
      {
        Sample::Line result;
        result.Level = src.GetLevel();
        result.VolSlide = src.GetVolSlide();
        result.Noise = src.GetNoise();
        result.AccumulateNoise = src.GetAccumulateNoise();
        result.NoiseMask = src.GetNoiseMask();
        result.Tone = src.GetTone();
        result.AccumulateTone = src.GetAccumulateTone();
        result.ToneMask = src.GetToneMask();
        result.EnvelopeAddon = src.GetEnvelopeAddon();
        result.AccumulateEnvelope = src.GetAccumulateEnvelope();
        result.EnableEnvelope = src.GetEnableEnvelope();
        return result;
      }

      static void ParseOrnament(const RawOrnament& src, uint_t size, Ornament& dst)
      {
        Require(size <= MAX_ORNAMENT_SIZE);
        dst.Lines.resize(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawOrnament::Line& line = src.GetLine(idx);
          dst.Lines[idx] = ParseOrnamentLine(line);
        }
        dst.Loop = std::min<uint_t>(src.GetLoop(), dst.Lines.size());
      }

      static Ornament::Line ParseOrnamentLine(const RawOrnament::Line& src)
      {
        Ornament::Line result;
        result.NoteAddon = src.GetNoteAddon();
        result.KeepNoteAddon = src.GetKeepNoteAddon();
        result.NoiseAddon = src.GetNoiseAddon();
        result.KeepNoiseAddon = src.GetKeepNoiseAddon();
        return result;
      }
    private:
      const Binary::TypedContainer& Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
      const std::size_t BaseAddr;
    };

    enum AreaTypes
    {
      HEADER,
      POSITIONS,
      PATTERNS,
      PATTERNS_DATA,
      SAMPLES,
      ORNAMENTS,
      END
    };

    class Areas : public AreaController
    {
    public:
      explicit Areas(const Binary::TypedContainer& data)
        : BaseAddr(0)
      {
        const RawHeader& header = *data.GetField<RawHeader>(0);
        const std::size_t firstPatternOffset = fromLE(header.PatternsOffset);
        AddArea(HEADER, 0);
        AddArea(POSITIONS, sizeof(header));
        AddArea(PATTERNS, firstPatternOffset);
        AddArea(END, data.GetSize());
        if (GetAreaSize(PATTERNS) < sizeof(RawPattern))
        {
          return;
        }
        const std::size_t firstOrnamentOffset = fromLE(header.OrnamentsOffsets[0]);
        const std::size_t firstSampleOffset = fromLE(header.SamplesOffsets[0]);
        if (firstSampleOffset >= firstOrnamentOffset)
        {
          return;
        }
        std::size_t patternDataAddr = 0;
        for (std::size_t pos = firstPatternOffset; ; pos += sizeof(RawPattern))
        {
          if (const RawPattern* pat = data.GetField<RawPattern>(pos))
          {
            if (pat->Offsets[0] == 0xffff)
            {
              const std::size_t patternDataOffset = pos + sizeof(pat->Offsets[0]);
              if (patternDataAddr >= patternDataOffset)
              {
                BaseAddr = patternDataAddr - patternDataOffset;
                AddArea(PATTERNS_DATA, patternDataOffset);
                break;
              }
              else
              {
                return;
              }
            }
            else if (patternDataAddr == 0 && pat->Offsets[0] != 0)
            {
              patternDataAddr = fromLE(pat->Offsets[0]);
            }
          }
          else
          {
            return;
          }
        }
        if (firstSampleOffset > BaseAddr && data.GetField<RawOrnament>(firstSampleOffset - BaseAddr))
        {
          AddArea(SAMPLES, firstSampleOffset - BaseAddr);
        }
        if (firstOrnamentOffset > BaseAddr && data.GetField<RawOrnament>(firstOrnamentOffset - BaseAddr))
        {
          AddArea(ORNAMENTS, firstOrnamentOffset - BaseAddr);
        }
      }

      bool CheckHeader() const
      {
        if (sizeof(RawHeader) > GetAreaSize(HEADER) || Undefined != GetAreaSize(END))
        {
          return false;
        }
        return true;
      }

      bool CheckPositions() const
      {
        const std::size_t size = GetAreaSize(POSITIONS);
        if (Undefined == size)
        {
          return false;
        }
        if (size < sizeof(RawPosition) + 1)
        {
          return false;
        }
        return true;
      }

      bool CheckPatterns() const
      {
        const std::size_t size = GetAreaSize(PATTERNS);
        return size != Undefined && size >= sizeof(RawPattern) + sizeof(uint16_t);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES);
        return size != Undefined && size >= sizeof(RawSample);
      }

      bool CheckOrnaments() const
      {
        const std::size_t size = GetAreaSize(ORNAMENTS);
        return size != Undefined && size >= sizeof(RawOrnament);
      }

      std::size_t GetBaseAddress() const
      {
        return BaseAddr;
      }
    private:
      std::size_t BaseAddr;
    };

    bool FastCheck(const Areas& areas)
    {
      if (!areas.CheckHeader())
      {
        return false;
      }
      if (!areas.CheckPositions())
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
      return true;
    }

    bool FastCheck(const Binary::TypedContainer& data)
    {
      const Areas areas(data);
      return FastCheck(areas);
    }

    Binary::TypedContainer CreateContainer(const Binary::Container& data)
    {
      return Binary::TypedContainer(data, std::min(data.Size(), MAX_MODULE_SIZE));
    }

    const std::string FORMAT(
      "?{8}"         //identifier
      "?{42}"        //title
      "?"            //semicolon
      "?{18}"        //editor
      "03-ff"        //tempo
      "00-fe"        //loop
      "?{4}"         //padding1
      "?00-02"       //patterns offset
      "?{5}"         //padding2
      "(?03-2c|64-ff){32}" //samples
      "(?05-2d|66-ff){33}" //ornaments
      "00-1f?"       //at least one position
      "ff|00-1f"     //next position or end
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
        return Text::FASTTRACKER_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData) && FastCheck(CreateContainer(rawData));
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

      const Areas areas(data);
      if (!FastCheck(areas))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        const Format format(data, areas.GetBaseAddress());
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
  }//namespace FastTracker

  Decoder::Ptr CreateFastTrackerDecoder()
  {
    return boost::make_shared<FastTracker::Decoder>();
  }
}//namespace Chiptune
}//namespace Formats
