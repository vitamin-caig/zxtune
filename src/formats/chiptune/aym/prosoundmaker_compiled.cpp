/**
 *
 * @file
 *
 * @brief  ProSoundMaker compiled modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/prosoundmaker.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <make_ptr.h>
#include <pointers.h>
#include <range_checker.h>
#include <string_view.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/optimize.h>
// std includes
#include <array>

namespace Formats::Chiptune
{
  namespace ProSoundMakerCompiled
  {
    const Debug::Stream Dbg("Formats::Chiptune::ProSoundMakerCompiled");

    const Char PROGRAM[] = "Pro Sound Maker";

    using namespace ProSoundMaker;

    const std::size_t MIN_SIZE = 0x80;
    const std::size_t MAX_SIZE = 0x4000;
    const std::size_t MAX_POSITIONS_COUNT = 100;
    const std::size_t MIN_PATTERN_SIZE = 1;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_SAMPLES_COUNT = 15;
    const std::size_t MAX_ORNAMENTS_COUNT = 32;
    // speed=2..50
    // modulate=-36..+36

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

    struct RawHeader
    {
      le_uint16_t PositionsOffset;
      le_uint16_t SamplesOffset;
      le_uint16_t OrnamentsOffset;
      le_uint16_t PatternsOffset;
    };

    const uint8_t ID1[] = {'p', 's', 'm', '1', 0};

    const uint8_t POS_END_MARKER = 0xff;

    struct RawObject
    {
      uint8_t Size;
      uint8_t Loop;
    };

    struct RawSample : RawObject
    {
      struct Line
      {
        // NxxTaaaa
        // nnnnnSHH
        // LLLLLLLL

        // SHHLLLLLLLL - shift
        // S - shift sign
        // a - level
        // N - noise off
        // T - tone off
        // n - noise value
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
      };

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
        const auto* const src = safe_ptr_cast<const uint8_t*>(this + 1);
        // using 8-bit offsets
        auto offset = static_cast<uint8_t>(idx * sizeof(Line));
        Line res;
        res.LevelAndFlags = src[offset++];
        res.NoiseHiShift = src[offset++];
        res.LoShift = src[offset++];
        return res;
      }
    };

    struct RawOrnament : RawObject
    {
      using Line = int8_t;

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
        const auto* const src = safe_ptr_cast<const int8_t*>(this + 1);
        // using 8-bit offsets
        auto offset = static_cast<uint8_t>(idx * sizeof(Line));
        return src[offset];
      }
    };

    struct RawPosition
    {
      uint8_t PatternIndex;
      uint8_t TranspositionOrLoop;
    };

    struct RawPattern
    {
      uint8_t Tempo;
      std::array<le_uint16_t, 3> Offsets;
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 8, "Invalid layout");
    static_assert(sizeof(RawSample) * alignof(RawSample) == 2, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 2, "Invalid layout");
    static_assert(sizeof(RawPosition) * alignof(RawPosition) == 2, "Invalid layout");
    static_assert(sizeof(RawPattern) * alignof(RawPattern) == 7, "Invalid layout");

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      void SetSample(uint_t /*index*/, Sample /*sample*/) override {}
      void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}
      void SetPositions(Positions /*positions*/) override {}
      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }
      virtual void StartLine(uint_t /*index*/) {}
      void StartChannel(uint_t /*index*/) override {}
      void SetRest() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetSample(uint_t /*sample*/) override {}
      void SetOrnament(uint_t /*ornament*/) override {}
      void SetVolume(uint_t /*volume*/) override {}
      void DisableOrnament() override {}
      void SetEnvelopeReinit(bool /*enabled*/) override {}
      void SetEnvelopeTone(uint_t /*type*/, uint_t /*tone*/) override {}
      void SetEnvelopeNote(uint_t /*type*/, uint_t /*note*/) override {}
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
        UsedSamples.Insert(0);
        UsedOrnaments.Insert(0);
      }

      MetaBuilder& GetMetaBuilder() override
      {
        return Delegate.GetMetaBuilder();
      }

      void SetSample(uint_t index, Sample sample) override
      {
        assert(index == 0 || UsedSamples.Contain(index));
        if (IsSampleSounds(sample))
        {
          NonEmptySamples = true;
        }
        return Delegate.SetSample(index, std::move(sample));
      }

      void SetOrnament(uint_t index, Ornament ornament) override
      {
        assert(index == 0 || UsedOrnaments.Contain(index));
        return Delegate.SetOrnament(index, std::move(ornament));
      }

      void SetPositions(Positions positions) override
      {
        UsedPatterns.Clear();
        for (const auto& pos : positions.Lines)
        {
          UsedPatterns.Insert(pos.PatternIndex);
        }
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(std::move(positions));
      }

      PatternBuilder& StartPattern(uint_t index) override
      {
        assert(UsedPatterns.Contain(index));
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
        NonEmptyPatterns = true;
        return Delegate.SetNote(note);
      }

      void SetSample(uint_t sample) override
      {
        UsedSamples.Insert(sample);
        return Delegate.SetSample(sample);
      }

      void SetOrnament(uint_t ornament) override
      {
        if (ornament != 0x20 && ornament != 0x21)
        {
          UsedOrnaments.Insert(ornament);
        }
        return Delegate.SetOrnament(ornament);
      }

      void SetVolume(uint_t volume) override
      {
        return Delegate.SetVolume(volume);
      }

      void DisableOrnament() override
      {
        return Delegate.DisableOrnament();
      }

      void SetEnvelopeReinit(bool enabled) override
      {
        return Delegate.SetEnvelopeReinit(enabled);
      }

      void SetEnvelopeTone(uint_t type, uint_t tone) override
      {
        return Delegate.SetEnvelopeTone(type, tone);
      }

      void SetEnvelopeNote(uint_t type, uint_t note) override
      {
        return Delegate.SetEnvelopeNote(type, note);
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
        // has envelope or tone with volume
        return std::any_of(smp.Lines.begin(), smp.Lines.end(),
                           [](const auto& line) { return !line.NoiseMask || (!line.ToneMask && line.Level); });
      }

    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
      Indices UsedOrnaments;
      bool NonEmptyPatterns = false;
      bool NonEmptySamples = false;
    };

    class RangesMap
    {
    public:
      explicit RangesMap(std::size_t limit)
        : ServiceRanges(RangeChecker::CreateShared(limit))
        , TotalRanges(RangeChecker::CreateSimple(limit))
        , FixedRanges(RangeChecker::CreateSimple(limit))
      {}

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
        Dbg(" Affected range {}..{}", offset, offset + size);
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
      explicit Format(const Binary::View data)
        : Data(data)
        , Ranges(data.Size())
        , Source(*Data.As<RawHeader>())
      {
        Ranges.AddService(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(PROGRAM);
        if (const std::size_t gapSize = Source.PositionsOffset - sizeof(Source))
        {
          const std::size_t gapBegin = sizeof(Source);
          const std::size_t gapEnd = gapBegin + gapSize;
          std::size_t titleBegin = gapBegin;
          if (gapSize >= sizeof(ID1) && std::equal(ID1, std::end(ID1), PeekObject<uint8_t>(gapBegin)))
          {
            titleBegin += sizeof(ID1);
          }
          if (titleBegin < gapEnd)
          {
            const auto* const titleStart = PeekObject<char>(titleBegin);
            const auto title = MakeStringView(titleStart, titleStart + gapEnd - titleBegin);
            meta.SetTitle(Strings::OptimizeAscii(title));
          }
          Ranges.AddService(gapBegin, gapSize);
        }
      }

      void ParsePositions(Builder& builder) const
      {
        Positions positions;
        for (uint_t entryIdx = 0;; ++entryIdx)
        {
          const RawPosition& pos = GetPosition(entryIdx);
          const uint_t patNum = pos.PatternIndex;
          if (patNum == POS_END_MARKER)
          {
            if (pos.TranspositionOrLoop == POS_END_MARKER)
            {
              break;
            }
            positions.Loop = pos.TranspositionOrLoop;
            Require(positions.GetLoop() < positions.GetSize());
            break;
          }
          PositionEntry res;
          res.PatternIndex = patNum;
          res.Transposition = static_cast<int8_t>(pos.TranspositionOrLoop);
          Require(Math::InRange<int_t>(res.Transposition, -36, 36));
          positions.Lines.push_back(res);
          Require(Math::InRange<std::size_t>(positions.GetSize(), 1, MAX_POSITIONS_COUNT));
        }
        Dbg("Positions: {} entries, loop to {}", positions.GetSize(), positions.GetLoop());
        builder.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: {} to parse", pats.Count());
        const std::size_t minOffset = Source.PatternsOffset + pats.Maximum() * sizeof(RawPattern);
        bool hasValidPatterns = false;
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          if (ParsePattern(patIndex, minOffset, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Dbg("Samples: {} to parse", samples.Count());
        const std::size_t minOffset = Source.SamplesOffset + (samples.Maximum() + 1) * sizeof(uint16_t);
        const std::size_t maxOffset = Data.Size();
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const std::size_t samOffset = GetSampleOffset(samIdx);
          Require(Math::InRange(samOffset, minOffset, maxOffset));
          const std::size_t availSize = maxOffset - samOffset;
          const auto* src = PeekObject<RawSample>(samOffset);
          Require(src != nullptr);
          const std::size_t usedSize = src->GetUsedSize();
          Sample result;
          if (usedSize <= availSize)
          {
            Dbg("Parse sample {}", samIdx);
            Ranges.Add(samOffset, usedSize);
            ParseSample(*src, src->GetSize(), result);
          }
          else
          {
            Dbg("Parse partial sample {}", samIdx);
            Ranges.Add(samOffset, availSize);
            const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawSample::Line);
            ParseSample(*src, availLines, result);
          }
          builder.SetSample(samIdx, std::move(result));
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: {} to parse", ornaments.Count());
        if (Source.OrnamentsOffset == Source.PatternsOffset)
        {
          Require(ornaments.Count() == 1);
          Dbg("No ornaments. Making stub");
          builder.SetOrnament(0, Ornament());
          return;
        }
        const std::size_t minOffset = Source.OrnamentsOffset + (ornaments.Maximum() + 1) * sizeof(uint16_t);
        const std::size_t maxOffset = Data.Size();
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Ornament result;
          const std::size_t ornOffset = GetOrnamentOffset(ornIdx);
          Require(Math::InRange(ornOffset, minOffset, maxOffset));
          const std::size_t availSize = Data.Size() - ornOffset;
          const auto* src = PeekObject<RawOrnament>(ornOffset);
          Require(src != nullptr);
          const std::size_t usedSize = src->GetUsedSize();
          if (usedSize <= availSize)
          {
            Dbg("Parse ornament {}", ornIdx);
            Ranges.Add(ornOffset, usedSize);
            ParseOrnament(*src, src->GetSize(), result);
          }
          else
          {
            Dbg("Parse partial ornament {}", ornIdx);
            Ranges.Add(ornOffset, availSize);
            const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawOrnament::Line);
            ParseOrnament(*src, availLines, result);
          }
          builder.SetOrnament(ornIdx, std::move(result));
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
        return GetServiceObject<le_uint16_t>(samIdx, &RawHeader::SamplesOffset);
      }

      std::size_t GetOrnamentOffset(uint_t samIdx) const
      {
        return GetServiceObject<le_uint16_t>(samIdx, &RawHeader::OrnamentsOffset);
      }

      template<class T>
      const T* PeekObject(std::size_t offset) const
      {
        return Data.SubView(offset).As<T>();
      }

      template<class T>
      const T& GetServiceObject(uint_t idx, le_uint16_t RawHeader::*start) const
      {
        const std::size_t offset = Source.*start + idx * sizeof(T);
        Ranges.AddService(offset, sizeof(T));
        return *PeekObject<T>(offset);
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const auto* data = PeekObject<uint8_t>(offset);
        Require(data != nullptr);
        return *data;
      }

      struct DataCursors : public std::array<std::size_t, 3>
      {
        explicit DataCursors(const RawPattern& src)
        {
          std::copy(src.Offsets.begin(), src.Offsets.end(), begin());
        }
      };

      struct ChannelState
      {
        ChannelState() = default;

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

        std::size_t Offset = 0;
        uint_t Period = 0;
        uint_t Counter = 0;
        uint_t Note = 0;
      };

      struct ParserState
      {
        std::array<ChannelState, 3> Channels;

        explicit ParserState(const DataCursors& src)
        {
          for (uint_t idx = 0; idx != src.size(); ++idx)
          {
            Channels[idx].Note = 48;
            Channels[idx].Offset = src[idx];
          }
        }

        uint_t GetMinCounter() const
        {
          return std::min_element(
                     Channels.begin(), Channels.end(),
                     [](const ChannelState& lh, const ChannelState& rh) { return lh.Counter < rh.Counter; })
              ->Counter;
        }

        void SkipLines(uint_t toSkip)
        {
          for (auto& chan : Channels)
          {
            chan.Counter -= toSkip;
          }
        }
      };

      bool ParsePattern(uint_t patIndex, std::size_t minOffset, Builder& builder) const
      {
        const RawPattern& pat = GetPattern(patIndex);
        Require(Math::InRange<uint_t>(pat.Tempo, 2, 50));
        const DataCursors rangesStarts(pat);
        ParserState state(rangesStarts);
        // check only lower bound due to possible detection lack
        Require(std::all_of(rangesStarts.begin(), rangesStarts.end(), [minOffset](auto b) { return b >= minOffset; }));

        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        patBuilder.StartLine(0);
        patBuilder.SetTempo(pat.Tempo);
        uint_t lineIdx = 0;
        for (; lineIdx < MAX_PATTERN_SIZE; ++lineIdx)
        {
          // skip lines if required
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
          if (0 != lineIdx)
          {
            patBuilder.StartLine(lineIdx);
          }
          ParseLine(state, builder);
        }
        for (uint_t chanNum = 0; chanNum != rangesStarts.size(); ++chanNum)
        {
          const std::size_t start = rangesStarts[chanNum];
          if (start >= Data.Size())
          {
            Dbg("Invalid offset ({})", start);
          }
          else
          {
            const std::size_t stop = std::min(Data.Size(), state.Channels[chanNum].Offset + 1);
            Ranges.AddFixed(start, stop - start);
          }
        }
        return lineIdx >= MIN_PATTERN_SIZE;
      }

      bool HasLine(const ParserState& src) const
      {
        for (uint_t idx = 0; idx < 3; ++idx)
        {
          const ChannelState& chan = src.Channels[idx];
          if (chan.Counter)
          {
            continue;
          }
          if (chan.Offset >= Data.Size() || PeekByte(chan.Offset) >= 0xfc)
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
          if (chan.Counter)
          {
            --chan.Counter;
            continue;
          }
          builder.StartChannel(idx);
          ParseChannel(chan, builder);
          chan.Counter = chan.Period;
        }
      }

      void ParseChannel(ChannelState& state, Builder& builder) const
      {
        while (state.Offset < Data.Size())
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
            // 0x00-0x1f
            builder.SetOrnament(cmd - 0x70);
          }
          else if (cmd == 0x90)
          {
            break;
          }
          else if (cmd <= 0x9f)
          {
            // 1..f
            builder.SetVolume(cmd - 0x90);
          }
          else if (cmd == 0xa0)
          {
            builder.DisableOrnament();
          }
          else if (cmd <= 0xb0)
          {
            // 0..f
            const uint_t packedEnvelope = cmd - 0xa1;
            const uint_t envelopeType = 8 + ((packedEnvelope & 3) << 1);
            const uint_t envelopeNote = 3 * (packedEnvelope & 12);
            builder.SetEnvelopeNote(envelopeType, envelopeNote);
          }
          else if (cmd <= 0xb7)
          {
            // 8...e
            const uint_t envelopeType = cmd - 0xA9;
            const uint_t packedTone = PeekByte(state.Offset++);
            const uint_t envelopeTone = packedTone >= 0xf1 ? 256 * (packedTone & 15) : packedTone;
            builder.SetEnvelopeTone(envelopeType, envelopeTone);
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
            builder.SetEnvelopeReinit(cmd == 0xfb);
          }
          else
          {
            break;  // end of pattern
          }
        }
      }

      static void ParseSample(const RawSample& src, uint_t size, Sample& dst)
      {
        dst.Lines.reserve(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          dst.Lines.emplace_back(ParseSampleLine(line));
        }
        if (const uint_t loopCounter = src.GetLoopCounter())
        {
          dst.VolumeDeltaPeriod = loopCounter;
          dst.VolumeDeltaValue = src.GetVolumeDelta();
          dst.Loop = std::min<uint_t>(src.GetLoop(), dst.Lines.size());
        }
        else
        {
          dst.Loop = dst.Lines.size();
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
        else
        {
          dst.Loop = dst.Lines.size();
        }
      }

    private:
      const Binary::View Data;
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

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        AddArea(POSITIONS, header.PositionsOffset);
        AddArea(SAMPLES, header.SamplesOffset);
        AddArea(ORNAMENTS, header.OrnamentsOffset);
        AddArea(PATTERNS, header.PatternsOffset);
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return GetAreaSize(HEADER) >= sizeof(RawHeader) && Undefined == GetAreaSize(END);
      }

      std::size_t GetPositionsCount() const
      {
        const std::size_t positionsSize = GetAreaSize(POSITIONS);
        return Undefined != positionsSize && positionsSize > 0 && 0 == positionsSize % sizeof(RawPosition)
                   ? positionsSize / sizeof(RawPosition) - 1
                   : 0;
      }

      bool CheckSamples() const
      {
        const std::size_t samplesSize = GetAreaSize(SAMPLES);
        return Undefined != samplesSize && samplesSize != 0;
      }
    };

    Binary::View MakeContainer(Binary::View rawData)
    {
      return rawData.SubView(0, MAX_SIZE);
    }

    bool FastCheck(Binary::View data)
    {
      const auto& hdr = *data.As<RawHeader>();
      const Areas areas(hdr, data.Size());
      if (!areas.CheckHeader())
      {
        return false;
      }
      if (const std::size_t posCount = areas.GetPositionsCount())
      {
        const auto& lastPos =
            *data.SubView(areas.GetAreaAddress(POSITIONS) + posCount * sizeof(RawPosition)).As<RawPosition>();
        if (lastPos.PatternIndex != POS_END_MARKER)
        {
          return false;
        }
        if (lastPos.TranspositionOrLoop != POS_END_MARKER && lastPos.TranspositionOrLoop >= posCount)
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

    const auto DESCRIPTION = PROGRAM;
    // Statistic-based format description (~55 modules)
    const auto FORMAT =
        "(08-88)&%x0xxxxxx 00"  // uint16_t PositionsOffset;
        // 0x9d + 2 * MAX_POSITIONS_COUNT(0x64) = 0x165
        "? 00-01"  // uint16_t SamplesOffset;
        // 0x165 + MAX_SAMPLES_COUNT(0xf) * (2 + 2 + 3 * MAX_SAMPLE_SIZE(0x22)) = 0x79b
        "? 00-04"  // uint16_t OrnamentsOffset;
        // 0x79b + MAX_ORNAMENTS_COUNT(0x20) * (2 + 2 + MAX_ORNAMENT_SIZE(0x22)) = 0xc1b
        "? 00-05"  // uint16_t PatternsOffset;
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
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
        const auto data = MakeContainer(rawData);
        return Format->Match(data) && FastCheck(data);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return {};
        }
        Builder& stub = GetStubBuilder();
        return ParseCompiled(rawData, stub);
      }

    private:
      const Binary::Format::Ptr Format;
    };

  }  // namespace ProSoundMakerCompiled

  namespace ProSoundMaker
  {
    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& rawData, Builder& target)
    {
      using namespace ProSoundMakerCompiled;
      const auto data = MakeContainer(rawData);

      if (!FastCheck(data))
      {
        return {};
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
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, statistic);
        Require(statistic.HasNonEmptySamples());
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

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
      static ProSoundMakerCompiled::StubBuilder stub;
      return stub;
    }
  }  // namespace ProSoundMaker

  Decoder::Ptr CreateProSoundMakerCompiledDecoder()
  {
    return MakePtr<ProSoundMakerCompiled::Decoder>();
  }
}  // namespace Formats::Chiptune
