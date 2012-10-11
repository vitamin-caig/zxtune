/*
Abstract:
  ProTrackerMaker format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "prosoundmaker.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <debug_log.h>
#include <range_checker.h>
//library includes
#include <binary/typed_container.h>
#include <math/numeric.h>
//std includes
#include <set>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::ProSoundMakerCompiled");
}

namespace Formats
{
namespace Chiptune
{
  namespace ProSoundMakerCompiled
  {
    using namespace ProSoundMaker;

    const std::size_t MIN_SIZE = 256;
    const std::size_t MAX_SIZE = 0x3600;
    const std::size_t MAX_POSITIONS_COUNT = 100;
    const std::size_t MIN_PATTERN_SIZE = 1;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_SAMPLES_COUNT = 15;
    const std::size_t MAX_ORNAMENTS_COUNT = 32;
    //speed=2..50
    //modulate=-36..+36

    /*
      Typical module structure

      Header
      Id1 ("psm1,\x0", optional)
      Id2 (title, optional)
      Positions
      SamplesOffsets
      Samples
      OrnamentsOffsets (optional)
      Ornaments (optional)
      Patterns
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint16_t PositionsOffset;
      uint16_t SamplesOffset;
      uint16_t OrnamentsOffset;
      uint16_t PatternsOffset;
    } PACK_POST;

    const uint8_t ID1[] = {'p', 's', 'm', '1', 0};

    const uint8_t POS_END_MARKER = 0xff;

    PACK_PRE struct RawObject
    {
      uint8_t Size;
      uint8_t Loop;
    } PACK_POST;

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        //NxxTaaaa
        //nnnnnSHH
        //LLLLLLLL

        //SHHLLLLLLLL - shift
        //S - shift sign
        //a - level
        //N - noise off
        //T - tone off
        //n - noise value
        uint8_t LevelAndFlags;
        uint8_t NoiseHiShift;
        uint8_t LoShift;

        bool GetNoiseMask() const
        {
          return 0 != (LevelAndFlags & 128);
        }

        bool GetToneMask() const
        {
          return 0 != (LevelAndFlags & 16);
        }

        uint_t GetNoise() const
        {
          return NoiseHiShift >> 3;
        }

        uint_t GetLevel() const
        {
          return LevelAndFlags & 15;
        }

        int_t GetShift() const
        {
          const uint_t val(((NoiseHiShift & 0x07) << 8) | LoShift);
          return (NoiseHiShift & 4) ? static_cast<int16_t>(val | 0xf800) : val;
        }
      } PACK_POST;

      uint_t GetSize() const
      {
        return 1 + (Size & 0x1f);
      }

      uint_t GetLoopCounter() const
      {
        return (Loop & 0xe0) >> 5;
      }

      uint_t GetLoop() const
      {
        Require(GetLoopCounter() != 0);
        return Loop & 0x1f;
      }

      int_t GetVolumeDelta() const
      {
        const int_t val = (Size & 0xc0) >> 6;
        return Size & 0x20 ? -val - 1 : val;
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }

      Line GetLine(uint_t idx) const
      {
        const uint8_t* const src = safe_ptr_cast<const uint8_t*>(this + 1);
        //using 8-bit offsets
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        Line res;
        res.LevelAndFlags = src[offset++];
        res.NoiseHiShift = src[offset++];
        res.LoShift = src[offset++];
        return res;
      }
    } PACK_POST;

    PACK_PRE struct RawOrnament : RawObject
    {
      typedef int8_t Line;

      uint_t GetSize() const
      {
        return Size + 1;
      }

      bool HasLoop() const
      {
        return 0 != (Loop & 128);
      }

      uint_t GetLoop() const
      {
        Require(HasLoop());
        return Loop & 31;
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + GetSize() * sizeof(Line);
      }

      Line GetLine(uint_t idx) const
      {
        const int8_t* const src = safe_ptr_cast<const int8_t*>(this + 1);
        //using 8-bit offsets
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        return src[offset];
      }
    } PACK_POST;

    PACK_PRE struct RawPosition
    {
      uint8_t PatternIndex;
      uint8_t TranspositionOrLoop;
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      uint8_t Tempo;
      boost::array<uint16_t, 3> Offsets;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 8);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawPosition) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 7);

    class StubBuilder : public Builder
    {
    public:
      virtual void SetProgram(const String& /*program*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetSample(uint_t /*index*/, const Sample& /*sample*/) {}
      virtual void SetOrnament(uint_t /*index*/, const Ornament& /*ornament*/) {}
      virtual void SetPositions(const std::vector<PositionEntry>& /*positions*/, uint_t /*loop*/) {}
      virtual void StartPattern(uint_t /*index*/) {}
      virtual void SetTempo(uint_t /*tempo*/) {}
      virtual void FinishPattern(uint_t /*size*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetVolume(uint_t /*volume*/) {}
      virtual void DisableOrnament() {}
      virtual void SetEnvelopeType(uint_t /*type*/) {}
      virtual void SetEnvelopeTone(uint_t /*tone*/) {}
      virtual void SetEnvelopeNote(uint_t /*note*/) {}
    };

    typedef std::set<uint_t> Indices;

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , NonEmptyPatterns(false)
        , NonEmptySamples(false)
      {
      }

      virtual void SetProgram(const String& program)
      {
        return Delegate.SetProgram(program);
      }

      virtual void SetTitle(const String& title)
      {
        return Delegate.SetTitle(title);
      }

      virtual void SetSample(uint_t index, const Sample& sample)
      {
        assert(index == 0 || UsedSamples.count(index));
        if (IsSampleSounds(sample))
        {
          NonEmptySamples = true;
        }
        return Delegate.SetSample(index, sample);
      }

      virtual void SetOrnament(uint_t index, const Ornament& ornament)
      {
        assert(index == 0 || UsedOrnaments.count(index));
        return Delegate.SetOrnament(index, ornament);
      }

      virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop)
      {
        Indices pats;
        std::transform(positions.begin(), positions.end(), std::inserter(pats, pats.end()), boost::mem_fn(&PositionEntry::PatternIndex));
        UsedPatterns.swap(pats);
        return Delegate.SetPositions(positions, loop);
      }

      virtual void StartPattern(uint_t index)
      {
        assert(UsedPatterns.count(index));
        return Delegate.StartPattern(index);
      }

      virtual void SetTempo(uint_t tempo)
      {
        return Delegate.SetTempo(tempo);
      }

      virtual void FinishPattern(uint_t size)
      {
        return Delegate.FinishPattern(size);
      }

      virtual void StartLine(uint_t index)
      {
        return Delegate.StartLine(index);
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
        NonEmptyPatterns = true;
        return Delegate.SetNote(note);
      }

      virtual void SetSample(uint_t sample)
      {
        UsedSamples.insert(sample);
        return Delegate.SetSample(sample);
      }

      virtual void SetOrnament(uint_t ornament)
      {
        if (ornament != 0x20 && ornament != 0x21)
        {
          UsedOrnaments.insert(ornament);
        }
        return Delegate.SetOrnament(ornament);
      }

      virtual void SetVolume(uint_t volume)
      {
        return Delegate.SetVolume(volume);
      }

      virtual void DisableOrnament()
      {
        return Delegate.DisableOrnament();
      }

      virtual void SetEnvelopeType(uint_t type)
      {
        return Delegate.SetEnvelopeType(type);
      }

      virtual void SetEnvelopeTone(uint_t tone)
      {
        return Delegate.SetEnvelopeTone(tone);
      }

      virtual void SetEnvelopeNote(uint_t delta)
      {
        return Delegate.SetEnvelopeNote(delta);
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

      bool HasNonEmptyPatterns() const
      {
        return NonEmptyPatterns;
      }

      bool HasNonEmptySamples() const
      {
        return NonEmptySamples;
      }
    private:
      static bool IsSampleSounds(const Sample& smp)
      {
        if (smp.Lines.empty())
        {
          return false;
        }
        for (uint_t idx = 0; idx != smp.Lines.size(); ++idx)
        {
          const Sample::Line& line = smp.Lines[idx];
          if ((!line.ToneMask && line.Level) || !line.NoiseMask)
          {
            return true;//has envelope or tone with volume
          }
        }
        return false;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
      Indices UsedOrnaments;
      bool NonEmptyPatterns;
      bool NonEmptySamples;
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

    class Format
    {
    public:
      Format(const Binary::TypedContainer& data)
        : Delegate(data)
        , Ranges(data.GetSize())
        , Source(*Delegate.GetField<RawHeader>(0))
      {
        Ranges.AddService(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetProgram(Text::PROSOUNDMAKER_DECODER_DESCRIPTION);
        if (const std::size_t gapSize = fromLE(Source.PositionsOffset) - sizeof(Source))
        {
          const std::size_t gapBegin = sizeof(Source);
          const std::size_t gapEnd = gapBegin + gapSize;
          std::size_t titleBegin = gapBegin;
          if (gapSize >= sizeof(ID1) && std::equal(ID1, ArrayEnd(ID1), Delegate.GetField<uint8_t>(gapBegin)))
          {
            titleBegin += ArraySize(ID1);
          }
          if (titleBegin < gapEnd)
          {
            const char* const titleStart = Delegate.GetField<char>(titleBegin);
            const String title(titleStart, titleStart + gapEnd - titleBegin);
            builder.SetTitle(title);
          }
          Ranges.AddService(gapBegin, gapSize);
        }
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<PositionEntry> positions;
        uint_t loop = 0;
        for (uint_t entryIdx = 0; ; ++entryIdx)
        {
          const RawPosition& pos = GetPosition(entryIdx);
          const uint_t patNum = pos.PatternIndex;
          if (patNum == POS_END_MARKER)
          {
            if (pos.TranspositionOrLoop == POS_END_MARKER)
            {
              break;
            }
            loop = pos.TranspositionOrLoop;
            Require(loop < positions.size());
            break;
          }
          Require(Math::InRange<uint_t>(patNum, 0, MAX_PATTERNS_COUNT - 1));
          PositionEntry res;
          res.PatternIndex = patNum;
          res.Transposition = static_cast<int8_t>(pos.TranspositionOrLoop);
          Require(Math::InRange<int_t>(res.Transposition, -36, 36));
          positions.push_back(res);
          Require(Math::InRange<std::size_t>(positions.size(), 1, MAX_POSITIONS_COUNT));
        }
        builder.SetPositions(positions, loop);
        Dbg("Positions: %1% entries, loop to %2%", positions.size(), loop);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Require(!pats.empty());
        Dbg("Patterns: %1% to parse", pats.size());
        const std::size_t minOffset = fromLE(Source.PatternsOffset) + *pats.rbegin() * sizeof(RawPattern);
        bool hasValidPatterns = false;
        for (Indices::const_iterator it = pats.begin(), lim = pats.end(); it != lim; ++it)
        {
          const uint_t patIndex = *it;
          Require(Math::InRange<uint_t>(patIndex, 0, MAX_PATTERNS_COUNT - 1));
          Dbg("Parse pattern %1%", patIndex);
          const RawPattern& src = GetPattern(patIndex);
          Require(Math::InRange<uint_t>(src.Tempo, 2, 50));
          builder.StartPattern(patIndex);
          builder.StartLine(0);
          builder.SetTempo(src.Tempo);
          if (ParsePattern(src, minOffset, builder))
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
        const std::size_t minOffset = fromLE(Source.SamplesOffset) + *samples.rbegin() * sizeof(uint16_t);
        const std::size_t maxOffset = Delegate.GetSize();
        for (Indices::const_iterator it = samples.begin(), lim = samples.end(); it != lim; ++it)
        {
          const uint_t samIdx = *it;
          Require(Math::InRange<uint_t>(samIdx, 0, MAX_SAMPLES_COUNT - 1));
          Sample result;
          const std::size_t samOffset = GetSampleOffset(samIdx);
          Require(Math::InRange(samOffset, minOffset, maxOffset));
          const std::size_t availSize = maxOffset - samOffset;
          const RawSample* const src = Delegate.GetField<RawSample>(samOffset);
          Require(src != 0);
          const std::size_t usedSize = src->GetUsedSize();
          if (usedSize <= availSize)
          {
            Dbg("Parse sample %1%", samIdx);
            Ranges.Add(samOffset, usedSize);
            ParseSample(*src, src->GetSize(), result);
          }
          else
          {
            Dbg("Parse partial sample %1%", samIdx);
            Ranges.Add(samOffset, availSize);
            const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawSample::Line);
            ParseSample(*src, availLines, result);
          }
          builder.SetSample(samIdx, result);
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Require(!ornaments.empty() && 0 == *ornaments.begin());
        Dbg("Ornaments: %1% to parse", ornaments.size());
        if (Source.OrnamentsOffset == Source.PatternsOffset)
        {
          Require(ornaments.size() == 1);
          Dbg("No ornaments. Making stub");
          builder.SetOrnament(0, Ornament());
          return;
        }
        const std::size_t minOffset = fromLE(Source.OrnamentsOffset) + *ornaments.rbegin() * sizeof(uint16_t);
        const std::size_t maxOffset = Delegate.GetSize();
        for (Indices::const_iterator it = ornaments.begin(), lim = ornaments.end(); it != lim; ++it)
        {
          const uint_t ornIdx = *it;
          Require(Math::InRange<uint_t>(ornIdx, 0, MAX_ORNAMENTS_COUNT - 1));
          Ornament result;
          const std::size_t ornOffset = GetOrnamentOffset(ornIdx);
          Require(Math::InRange(ornOffset, minOffset, maxOffset));
          const std::size_t availSize = Delegate.GetSize() - ornOffset;
          const RawOrnament* src = Delegate.GetField<RawOrnament>(ornOffset);
          Require(src != 0);
          const std::size_t usedSize = src->GetUsedSize();
          if (usedSize <= availSize)
          {
            Dbg("Parse ornament %1%", ornIdx);
            Ranges.Add(ornOffset, usedSize);
            ParseOrnament(*src, src->GetSize(), result);
          }
          else
          {
            Dbg("Parse partial ornament %1%", ornIdx);
            Ranges.Add(ornOffset, availSize);
            const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawOrnament::Line);
            ParseOrnament(*src, availLines, result);
          }
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
      const RawPosition& GetPosition(uint_t entryIdx) const
      {
        return GetServiceObject<RawPosition>(entryIdx, &RawHeader::PositionsOffset);
      }

      const RawPattern& GetPattern(uint_t patIdx) const
      {
        return GetServiceObject<RawPattern>(patIdx, &RawHeader::PatternsOffset);
      }

      std::size_t GetSampleOffset(uint_t samIdx) const
      {
        return GetServiceObject<uint16_t>(samIdx, &RawHeader::SamplesOffset);
      }

      std::size_t GetOrnamentOffset(uint_t samIdx) const
      {
        return GetServiceObject<uint16_t>(samIdx, &RawHeader::OrnamentsOffset);
      }

      template<class T>
      const T& GetServiceObject(uint_t idx, uint16_t RawHeader::*start) const
      {
        const std::size_t offset = fromLE(Source.*start) + idx * sizeof(T);
        Ranges.AddService(offset, sizeof(T));
        return *Delegate.GetField<T>(offset);
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
          std::transform(src.Offsets.begin(), src.Offsets.end(), begin(), boost::bind(&fromLE<uint16_t>, _1));
        }
      };

      struct ChannelState
      {
        ChannelState()
          : Offset()
          , Period()
          , Counter()
          , Note()
        {
        }

        void UpdateNote(uint_t delta)
        {
          if (Note >= delta)
          {
            Note -= delta;
          }
          else
          {
            Note = (Note + 0x60 - delta) & 0xff;
          }
        }

        std::size_t Offset;
        uint_t Period;
        uint_t Counter;
        uint_t Note;
      };

      struct ParserState
      {
        boost::array<ChannelState, 3> Channels;

        explicit ParserState(const DataCursors& src)
          : Channels()
        {
          for (uint_t idx = 0; idx != src.size(); ++idx)
          {
            Channels[idx].Note = 48;
            Channels[idx].Offset = src[idx];
          }
        }

        uint_t GetMinCounter() const
        {
          return std::min_element(Channels.begin(), Channels.end(),
            boost::bind(&ChannelState::Counter, _1) < boost::bind(&ChannelState::Counter, _2))->Counter;
        }

        void SkipLines(uint_t toSkip)
        {
          for (uint_t idx = 0; idx != Channels.size(); ++idx)
          {
            Channels[idx].Counter -= toSkip;
          }
        }
      };

      bool ParsePattern(const RawPattern& pat, std::size_t minOffset, Builder& builder) const
      {
        const DataCursors rangesStarts(pat);
        //check only lower bound due to possible detection lack
        Require(rangesStarts.end() == std::find_if(rangesStarts.begin(), rangesStarts.end(), 
          std::bind2nd(std::less<std::size_t>(), minOffset)));

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
          if (lineIdx)
          {
            builder.StartLine(lineIdx);
          }
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
            const std::size_t stop = std::min(Delegate.GetSize(), state.Channels[chanNum].Offset + 1);
            Ranges.AddFixed(start, stop - start);
          }
        }
        return lineIdx >= MIN_PATTERN_SIZE;
      }

      bool HasLine(ParserState& src) const
      {
        for (uint_t idx = 0; idx < 3; ++idx)
        {
          const ChannelState& chan = src.Channels[idx];
          if (chan.Counter)
          {
            continue;
          }
          if (chan.Offset >= Delegate.GetSize())
          {
            return false;
          }
          else if (PeekByte(chan.Offset) >= 0xfc)
          {
            return false;
          }
        }
        return true;
      }

      void ParseLine(ParserState& src, Builder& builder) const
      {
        for (uint_t idx = 0; idx < 3; ++idx)
        {
          ChannelState& chan = src.Channels[idx];
          if (chan.Counter--)
          {
            continue;
          }
          builder.StartChannel(idx);
          ParseChannel(chan, builder);
          chan.Counter = chan.Period;
        }
      }

      void ParseChannel(ChannelState& state, Builder& builder) const
      {
        while (state.Offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd <= 0x5f)
          {
            state.UpdateNote(cmd);
            builder.SetNote(state.Note & 0x7f);
            if (0 != (state.Note & 0x80))
            {
              builder.SetRest();
            }
            break;
          }
          else if (cmd == 0x60)
          {
            builder.SetRest();
            break;
          }
          else if (cmd <= 0x6f)
          {
            builder.SetSample(cmd - 0x61);
          }
          else if (cmd <= 0x8f)
          {
            //0x00-0x1f
            builder.SetOrnament(cmd - 0x70);
          }
          else if (cmd == 0x90)
          {
            break;
          }
          else if (cmd <= 0x9f)
          {
            //1..f
            builder.SetVolume(cmd - 0x90);
          }
          else if (cmd == 0xa0)
          {
            builder.DisableOrnament();
          }
          else if (cmd <= 0xb0)
          {
            builder.SetOrnament(0x21);
            //0..f
            const uint_t packedEnvelope = cmd - 0xa1;
            const uint_t envelopeType = 8 + ((packedEnvelope & 3) << 1);
            const uint_t envelopeNote = 3 * (packedEnvelope & 12);
            builder.SetEnvelopeType(envelopeType);
            builder.SetEnvelopeNote(envelopeNote);
          }
          else if (cmd <= 0xb7)
          {
            //8...e
            builder.SetEnvelopeType(cmd - 0xA9);
            const uint_t packedTone = PeekByte(state.Offset++);
            const uint_t tone = packedTone >= 0xf1 ? 256 * (packedTone & 15) : packedTone;
            builder.SetEnvelopeTone(tone);
          }
          else if (cmd <= 0xf8)
          {
            state.Period = cmd - 0xb7 - 1;
          }
          else if (cmd == 0xf9)
          {
            continue;
          }
          else if (cmd <= 0xfb)
          {
            //0x20/0x21
            builder.SetOrnament(cmd - 0xda);
          }
          else
          {
            break;//end of pattern
          }
        }
      }

      static void ParseSample(const RawSample& src, uint_t size, Sample& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          dst.Lines[idx] = ParseSampleLine(line);
        }
        if (const uint_t loopCounter = src.GetLoopCounter())
        {
          dst.VolumeDeltaPeriod = loopCounter;
          dst.VolumeDeltaValue = src.GetVolumeDelta();
          dst.Loop = std::min<uint_t>(src.GetLoop(), dst.Lines.size());
        }
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& line)
      {
        Sample::Line res;
        res.Level = line.GetLevel();
        res.Noise = line.GetNoise();
        res.ToneMask = line.GetToneMask();
        res.NoiseMask = line.GetNoiseMask();
        res.Gliss = line.GetShift();
        return res;
      }

      static void ParseOrnament(const RawOrnament& src, uint_t size, Ornament& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          dst.Lines[idx] = src.GetLine(idx);
        }
        if (src.HasLoop())
        {
          dst.Loop = std::min<uint_t>(src.GetLoop(), dst.Lines.size());
        }
      }
    private:
      const Binary::TypedContainer& Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
    };

    enum AreaTypes
    {
      HEADER,
      POSITIONS,
      SAMPLES,
      ORNAMENTS,
      PATTERNS,
      END
    };

    struct Areas : public AreaController<AreaTypes, 1 + END, std::size_t>
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        AddArea(POSITIONS, fromLE(header.PositionsOffset));
        AddArea(SAMPLES, fromLE(header.SamplesOffset));
        AddArea(ORNAMENTS, fromLE(header.OrnamentsOffset));
        AddArea(PATTERNS, fromLE(header.PatternsOffset));
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return GetAreaSize(HEADER) >= sizeof(RawHeader) && Undefined == GetAreaSize(END);
      }

      std::size_t GetPositionsCount() const
      {
        const std::size_t positionsSize = GetAreaSize(POSITIONS);
        return Undefined != positionsSize && 0 == positionsSize % sizeof(RawPosition)
          ? positionsSize / sizeof(RawPosition) - 1
          : 0;
      }

      bool CheckSamples() const
      {
        const std::size_t samplesSize = GetAreaSize(SAMPLES);
        return Undefined != samplesSize && samplesSize != 0;
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
      if (const std::size_t posCount = areas.GetPositionsCount())
      {
        const RawPosition& lastPos = *data.GetField<RawPosition>(areas.GetAreaAddress(POSITIONS) + posCount * sizeof(RawPosition));
        if (lastPos.PatternIndex != POS_END_MARKER)
        {
          return false;
        }
        if (lastPos.TranspositionOrLoop != POS_END_MARKER &&
            lastPos.TranspositionOrLoop >= posCount)
        {
          return false;
        }
      }
      else
      {
        return false;
      }
      if (!areas.CheckSamples())
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
      "08-9d 00"  //uint16_t PositionsOffset;
      //0x9d + 7 * MAX_POSITIONS_COUNT = 359
      "? 00-03"      //uint16_t SamplesOffset;
      //0x2F9 + MAX_SAMPLES_COUNT * (2 + 2 + 3 * MAX_SAMPLE_SIZE) = 935
      "? 00-09"       //uint16_t OrnamentsOffset;
      //0x8d5 + MAX_ORNAMENTS_COUNT * (2 + 2 + MAX_ORNAMENT_SIZE) = d75
      "? 00-0d" //uint16_t PatternsOffset;
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
        return Text::PROSOUNDMAKER_DECODER_DESCRIPTION;
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
        return ParseCompiled(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

  }//ProSoundMakerCompiled

  namespace ProSoundMaker
  {
    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& rawData, Builder& target)
    {
      using namespace ProSoundMakerCompiled;
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
        Require(statistic.HasNonEmptyPatterns());
        Indices usedSamples = statistic.GetUsedSamples();
        usedSamples.insert(0);
        format.ParseSamples(usedSamples, statistic);
        Require(statistic.HasNonEmptySamples());
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

    Builder& GetStubBuilder()
    {
      static ProSoundMakerCompiled::StubBuilder stub;
      return stub;
    }
  }// namespace ProSoundMakerCompiled

  Decoder::Ptr CreateProSoundMakerCompiledDecoder()
  {
    return boost::make_shared<ProSoundMakerCompiled::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats
