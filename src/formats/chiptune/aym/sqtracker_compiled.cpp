/**
 *
 * @file
 *
 * @brief  SQTracker compiled modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/sqtracker.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <make_ptr.h>
#include <range_checker.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
// std includes
#include <array>

namespace Formats::Chiptune
{
  namespace SQTracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::SQTracker");

    const Char PROGRAM[] = "SQ-Tracker";

    const std::size_t MIN_MODULE_SIZE = 256;
    const std::size_t MAX_MODULE_SIZE = 0x3600;
    // const std::size_t MAX_POSITIONS_COUNT = 120;
    const std::size_t MIN_PATTERN_SIZE = 11;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 99;
    const std::size_t MAX_SAMPLES_COUNT = 26;
    const std::size_t MAX_ORNAMENTS_COUNT = 26;
    const std::size_t SAMPLE_SIZE = 32;
    const std::size_t ORNAMENT_SIZE = 32;

    struct RawSample
    {
      uint8_t Loop;
      uint8_t LoopSize;
      struct Line
      {
        // nnnnvvvv
        uint8_t VolumeAndNoise;
        // nTNDdddd
        // n- low noise bit
        // d- high delta bits
        // D- delta sign (1 for positive)
        // N- enable noise
        // T- enable tone
        uint8_t Flags;
        // dddddddd - low delta bits
        uint8_t Delta;

        uint_t GetLevel() const
        {
          return VolumeAndNoise & 15;
        }

        uint_t GetNoise() const
        {
          return ((VolumeAndNoise & 240) >> 3) | ((Flags & 128) >> 7);
        }

        bool GetEnableNoise() const
        {
          return 0 != (Flags & 32);
        }

        bool GetEnableTone() const
        {
          return 0 != (Flags & 64);
        }

        int_t GetToneDeviation() const
        {
          const int delta = int_t(Delta) | ((Flags & 15) << 8);
          return 0 != (Flags & 16) ? delta : -delta;
        }
      };
      Line Data[SAMPLE_SIZE];
    };

    struct RawPosEntry
    {
      struct Channel
      {
        // 0 is end
        // Eppppppp
        // p - pattern idx
        // E - enable effect
        uint8_t Pattern;
        // TtttVVVV
        // signed transposition
        uint8_t TranspositionAndAttenuation;

        uint_t GetPattern() const
        {
          return Pattern & 127;
        }

        int_t GetTransposition() const
        {
          const int_t transpos = TranspositionAndAttenuation >> 4;
          return transpos < 9 ? transpos : -(transpos - 9) - 1;
        }

        uint_t GetAttenuation() const
        {
          return TranspositionAndAttenuation & 15;
        }

        bool GetEnableEffects() const
        {
          return 0 != (Pattern & 128);
        }
      };

      Channel ChannelC;
      Channel ChannelB;
      Channel ChannelA;
      uint8_t Tempo;
    };

    struct RawOrnament
    {
      uint8_t Loop;
      uint8_t LoopSize;
      std::array<int8_t, ORNAMENT_SIZE> Lines;
    };

    struct RawHeader
    {
      le_uint16_t Size;
      le_uint16_t SamplesOffset;
      le_uint16_t OrnamentsOffset;
      le_uint16_t PatternsOffset;
      le_uint16_t PositionsOffset;
      le_uint16_t LoopPositionOffset;
    };

    static_assert(sizeof(RawSample::Line) * alignof(RawSample::Line) == 3, "Invalid layout");
    static_assert(sizeof(RawSample) * alignof(RawSample) == 98, "Invalid layout");
    static_assert(sizeof(RawPosEntry) * alignof(RawPosEntry) == 7, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 34, "Invalid layout");
    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 12, "Invalid layout");

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      // samples+ornaments
      void SetSample(uint_t /*index*/, Sample /*sample*/) override {}
      void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}
      // patterns
      void SetPositions(Positions /*positions*/) override {}
      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }
      void SetTempoAddon(uint_t /*add*/) override {}
      void SetRest() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetSample(uint_t /*sample*/) override {}
      void SetOrnament(uint_t /*ornament*/) override {}
      void SetEnvelope(uint_t /*type*/, uint_t /*value*/) override {}
      void SetGlissade(int_t /*step*/) override {}
      void SetAttenuation(uint_t /*att*/) override {}
      void SetAttenuationAddon(int_t /*add*/) override {}
      void SetGlobalAttenuation(uint_t /*att*/) override {}
      void SetGlobalAttenuationAddon(int_t /*add*/) override {}
    };

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(1, MAX_PATTERNS_COUNT)
        , UsedSamples(1, MAX_SAMPLES_COUNT)
        , UsedOrnaments(1, MAX_ORNAMENTS_COUNT)
      {}

      MetaBuilder& GetMetaBuilder() override
      {
        return Delegate.GetMetaBuilder();
      }

      void SetSample(uint_t index, Sample sample) override
      {
        assert(UsedSamples.Contain(index));
        return Delegate.SetSample(index, std::move(sample));
      }

      void SetOrnament(uint_t index, Ornament ornament) override
      {
        assert(UsedOrnaments.Contain(index));
        return Delegate.SetOrnament(index, std::move(ornament));
      }

      void SetPositions(Positions positions) override
      {
        UsedPatterns.Clear();
        for (const auto& pos : positions.Lines)
        {
          for (const auto& chan : pos.Channels)
          {
            UsedPatterns.Insert(chan.Pattern);
          }
        }
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(std::move(positions));
      }

      PatternBuilder& StartPattern(uint_t index) override
      {
        assert(UsedPatterns.Contain(index));
        return Delegate.StartPattern(index);
      }

      void SetTempoAddon(uint_t add) override
      {
        return Delegate.SetTempoAddon(add);
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

      void SetOrnament(uint_t ornament) override
      {
        UsedOrnaments.Insert(ornament);
        return Delegate.SetOrnament(ornament);
      }

      void SetEnvelope(uint_t type, uint_t value) override
      {
        return Delegate.SetEnvelope(type, value);
      }

      void SetGlissade(int_t step) override
      {
        return Delegate.SetGlissade(step);
      }

      void SetAttenuation(uint_t att) override
      {
        return Delegate.SetAttenuation(att);
      }

      void SetAttenuationAddon(int_t add) override
      {
        return Delegate.SetAttenuationAddon(add);
      }

      void SetGlobalAttenuation(uint_t att) override
      {
        return Delegate.SetGlobalAttenuation(att);
      }

      void SetGlobalAttenuationAddon(int_t add) override
      {
        return Delegate.SetGlobalAttenuationAddon(add);
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
        : ServiceRanges(RangeChecker::CreateSimple(limit))
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

    std::size_t GetFixDelta(const RawHeader& hdr)
    {
      const uint_t samplesAddr = hdr.SamplesOffset;
      // since ornaments,samples and patterns tables are 1-based, real delta is 2 bytes shorter
      const uint_t delta = offsetof(RawHeader, LoopPositionOffset);
      Require(samplesAddr >= delta);
      return samplesAddr - delta;
    }

    class Format
    {
    public:
      explicit Format(Binary::View data)
        : Data(data)
        , Ranges(Data.Size())
        , Source(*Data.As<RawHeader>())
        , Delta(GetFixDelta(Source))
      {
        Ranges.AddService(0, sizeof(Source));
      }

      static void ParseCommonProperties(Builder& builder)
      {
        builder.GetMetaBuilder().SetProgram(PROGRAM);
      }

      void ParsePositions(Builder& builder) const
      {
        const std::size_t loopPositionOffset = Source.LoopPositionOffset - Delta;
        std::size_t posOffset = Source.PositionsOffset - Delta;
        Positions result;
        for (uint_t pos = 0;; ++pos)
        {
          if (posOffset == loopPositionOffset)
          {
            result.Loop = pos;
          }
          if (const auto* fullEntry = PeekObject<RawPosEntry>(posOffset))
          {
            Ranges.AddService(posOffset, sizeof(*fullEntry));
            if (0 == fullEntry->ChannelC.GetPattern() || 0 == fullEntry->ChannelB.GetPattern()
                || 0 == fullEntry->ChannelA.GetPattern())
            {
              break;
            }
            PositionEntry dst;
            ParsePositionChannel(fullEntry->ChannelC, dst.Channels[2]);
            ParsePositionChannel(fullEntry->ChannelB, dst.Channels[1]);
            ParsePositionChannel(fullEntry->ChannelA, dst.Channels[0]);
            dst.Tempo = fullEntry->Tempo;
            posOffset += sizeof(*fullEntry);
            result.Lines.push_back(dst);
          }
          else
          {
            const auto& partialEntry = GetServiceObject<RawPosEntry::Channel>(posOffset);
            Require(0 == partialEntry.GetPattern());
            const std::size_t tailSize = std::min(sizeof(RawPosEntry), Data.Size() - posOffset);
            Ranges.Add(posOffset, tailSize);
            break;
          }
        }
        Dbg("Positions: {} entries, loop to {}", result.GetSize(), result.GetLoop());
        builder.SetPositions(std::move(result));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          ParsePattern(patIndex, builder);
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Dbg("Parse sample {}", samIdx);
          const RawSample& src = GetSample(samIdx);
          builder.SetSample(samIdx, ParseSample(src));
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        if (ornaments.Empty())
        {
          Dbg("No ornaments used");
          return;
        }
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Dbg("Parse ornament {}", ornIdx);
          const RawOrnament& src = GetOrnament(ornIdx);
          builder.SetOrnament(ornIdx, ParseOrnament(src));
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
      const T* PeekObject(std::size_t offset) const
      {
        return Data.SubView(offset).As<T>();
      }

      template<class T>
      const T& GetServiceObject(std::size_t offset) const
      {
        const auto* src = PeekObject<T>(offset);
        Require(src != nullptr);
        Ranges.AddService(offset, sizeof(T));
        return *src;
      }

      template<class T>
      const T& GetObject(std::size_t offset) const
      {
        const auto* src = PeekObject<T>(offset);
        Require(src != nullptr);
        Ranges.Add(offset, sizeof(T));
        return *src;
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const auto* src = PeekObject<uint8_t>(offset);
        Require(src != nullptr);
        return *src;
      }

      std::size_t GetPatternOffset(uint_t index) const
      {
        const std::size_t entryAddr = Source.PatternsOffset + index * sizeof(uint16_t);
        Require(entryAddr >= Delta);
        const std::size_t patternAddr = GetServiceObject<le_uint16_t>(entryAddr - Delta);
        Require(patternAddr >= Delta);
        return patternAddr - Delta;
      }

      const RawSample& GetSample(uint_t index) const
      {
        const std::size_t entryAddr = Source.SamplesOffset + index * sizeof(uint16_t);
        Require(entryAddr >= Delta);
        const std::size_t sampleAddr = GetServiceObject<le_uint16_t>(entryAddr - Delta);
        Require(sampleAddr >= Delta);
        return GetObject<RawSample>(sampleAddr - Delta);
      }

      const RawOrnament& GetOrnament(uint_t index) const
      {
        const std::size_t entryAddr = Source.OrnamentsOffset + index * sizeof(uint16_t);
        Require(entryAddr >= Delta);
        const std::size_t ornamentAddr = GetServiceObject<le_uint16_t>(entryAddr - Delta);
        Require(ornamentAddr >= Delta);
        return GetObject<RawOrnament>(ornamentAddr - Delta);
      }

      struct ParserState
      {
        uint_t Counter;
        std::size_t Cursor;
        uint_t LastNote;
        std::size_t LastNoteStart;
        bool RepeatLastNote;

        ParserState(std::size_t cursor)
          : Counter()
          , Cursor(cursor)
          , LastNote()
          , LastNoteStart()
          , RepeatLastNote()
        {}
      };

      void ParsePattern(std::size_t patIndex, Builder& builder) const
      {
        const std::size_t patOffset = GetPatternOffset(patIndex);
        const std::size_t patSize = PeekByte(patOffset);
        Require(Math::InRange(patSize, MIN_PATTERN_SIZE, MAX_PATTERN_SIZE));

        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        ParserState state(patOffset + 1);
        uint_t lineIdx = 0;
        for (; lineIdx < patSize; ++lineIdx)
        {
          if (state.Counter)
          {
            --state.Counter;
            if (state.RepeatLastNote)
            {
              patBuilder.StartLine(lineIdx);
              ParseNote(state.LastNoteStart, patBuilder, builder);
            }
          }
          else
          {
            patBuilder.StartLine(lineIdx);
            ParseLine(state, patBuilder, builder);
          }
        }
        patBuilder.Finish(lineIdx);
        const std::size_t start = patOffset;
        if (start >= Data.Size())
        {
          Dbg("Invalid offset ({})", start);
        }
        else
        {
          const std::size_t stop = std::min(Data.Size(), state.Cursor);
          Ranges.AddFixed(start, stop - start);
        }
      }

      void ParseLine(ParserState& state, PatternBuilder& patBuilder, Builder& builder) const
      {
        state.RepeatLastNote = false;
        const uint_t cmd = PeekByte(state.Cursor++);
        if (cmd <= 0x5f)
        {
          state.LastNote = cmd;
          state.LastNoteStart = state.Cursor - 1;
          builder.SetNote(cmd);
          state.Cursor = ParseNoteParameters(state.Cursor, patBuilder, builder);
        }
        else if (cmd <= 0x6e)
        {
          ParseEffect(cmd - 0x60, PeekByte(state.Cursor++), patBuilder, builder);
        }
        else if (cmd == 0x6f)
        {
          builder.SetRest();
        }
        else if (cmd <= 0x7f)
        {
          builder.SetRest();
          ParseEffect(cmd - 0x6f, PeekByte(state.Cursor++), patBuilder, builder);
        }
        else if (cmd <= 0x9f)
        {
          Require(0 != state.LastNoteStart);
          const uint_t addon = cmd & 15;
          if (0 != (cmd & 16))
          {
            state.LastNote -= addon;
          }
          else
          {
            state.LastNote += addon;
          }
          builder.SetNote(state.LastNote);
          ParseNote(state.LastNoteStart, patBuilder, builder);
        }
        else if (cmd <= 0xbf)
        {
          state.Counter = cmd & 15;
          if (cmd & 16)
          {
            state.RepeatLastNote |= state.Counter != 0;
            ParseNote(state.LastNoteStart, patBuilder, builder);
          }
        }
        else
        {
          state.LastNoteStart = state.Cursor - 1;
          builder.SetSample(cmd & 31);
        }
      }

      void ParseNote(std::size_t cursor, PatternBuilder& patBuilder, Builder& builder) const
      {
        const uint_t cmd = PeekByte(cursor);
        if (cmd < 0x80)
        {
          ParseNoteParameters(cursor + 1, patBuilder, builder);
        }
        else
        {
          builder.SetSample(cmd & 31);
        }
      }

      std::size_t ParseNoteParameters(std::size_t start, PatternBuilder& patBuilder, Builder& builder) const
      {
        std::size_t cursor = start;
        const uint_t cmd = PeekByte(cursor++);
        if (0 != (cmd & 128))
        {
          if (const uint_t sample = (cmd >> 1) & 31)
          {
            builder.SetSample(sample);
          }
          if (0 != (cmd & 64))
          {
            const uint_t param = PeekByte(cursor++);
            if (const uint_t ornament = (param >> 4) | ((cmd & 1) << 4))
            {
              builder.SetOrnament(ornament);
            }
            if (const uint_t effect = param & 15)
            {
              ParseEffect(effect, PeekByte(cursor++), patBuilder, builder);
            }
          }
        }
        else
        {
          ParseEffect(cmd, PeekByte(cursor++), patBuilder, builder);
        }
        return cursor;
      }

      static void ParseEffect(uint_t code, uint_t param, PatternBuilder& patBuilder, Builder& builder)
      {
        switch (code - 1)
        {
        case 0:
          builder.SetAttenuation(param & 15);
          break;
        case 1:
          builder.SetAttenuationAddon(static_cast<int8_t>(param));
          break;
        case 2:
          builder.SetGlobalAttenuation(param);
          break;
        case 3:
          builder.SetGlobalAttenuationAddon(static_cast<int8_t>(param));
          break;
        case 4:
          patBuilder.SetTempo(param & 31 ? param & 31 : 32);
          break;
        case 5:
          builder.SetTempoAddon(param);
          break;
        case 6:
          builder.SetGlissade(-int_t(param));
          break;
        case 7:
          builder.SetGlissade(int_t(param));
          break;
        default:
          builder.SetEnvelope((code - 1) & 15, param);
          break;
        }
      }

      static void ParsePositionChannel(const RawPosEntry::Channel& src, PositionEntry::Channel& dst)
      {
        dst.Pattern = src.GetPattern();
        dst.Transposition = src.GetTransposition();
        dst.Attenuation = src.GetAttenuation();
        dst.EnabledEffects = src.GetEnableEffects();
      }

      static Sample ParseSample(const RawSample& src)
      {
        Sample dst;
        dst.Lines.resize(SAMPLE_SIZE);
        for (uint_t idx = 0; idx < SAMPLE_SIZE; ++idx)
        {
          const RawSample::Line& line = src.Data[idx];
          Sample::Line& res = dst.Lines[idx];
          res.Level = line.GetLevel();
          res.Noise = line.GetNoise();
          res.EnableNoise = line.GetEnableNoise();
          res.EnableTone = line.GetEnableTone();
          res.ToneDeviation = line.GetToneDeviation();
        }
        dst.Loop = std::min<uint_t>(src.Loop, SAMPLE_SIZE);
        dst.LoopLimit = std::min<uint_t>(dst.Loop + src.LoopSize, SAMPLE_SIZE);
        return dst;
      }

      static Ornament ParseOrnament(const RawOrnament& src)
      {
        Ornament dst;
        dst.Lines.assign(src.Lines.begin(), src.Lines.end());
        dst.Loop = std::min<uint_t>(src.Loop, ORNAMENT_SIZE);
        dst.LoopLimit = std::min<uint_t>(dst.Loop + src.LoopSize, ORNAMENT_SIZE);
        return dst;
      }

    private:
      const Binary::View Data;
      RangesMap Ranges;
      const RawHeader& Source;
      const std::size_t Delta;
    };

    enum AreaTypes
    {
      HEADER,
      SAMPLES_OFFSETS,
      ORNAMENTS_OFFSETS,
      PATTERNS_OFFSETS,
      SAMPLES,
      ORNAMENTS,
      PATTERNS,
      POSITIONS,
      LOOPED_POSITION,
      END
    };

    struct Areas : public AreaController
    {
      Areas(Binary::View data, std::size_t unfixDelta, std::size_t size)
      {
        const auto& hdr = *data.As<RawHeader>();
        AddArea(HEADER, 0);
        const std::size_t sample1AddrOffset = hdr.SamplesOffset + sizeof(uint16_t) - unfixDelta;
        AddArea(SAMPLES_OFFSETS, sample1AddrOffset);
        const std::size_t ornament1AddrOffset = hdr.OrnamentsOffset + sizeof(uint16_t) - unfixDelta;
        if (hdr.OrnamentsOffset != hdr.PatternsOffset)
        {
          AddArea(ORNAMENTS_OFFSETS, ornament1AddrOffset);
        }
        const std::size_t pattern1AddOffset = hdr.PatternsOffset + sizeof(uint16_t) - unfixDelta;
        AddArea(PATTERNS_OFFSETS, pattern1AddOffset);
        AddArea(POSITIONS, hdr.PositionsOffset - unfixDelta);
        if (hdr.PositionsOffset != hdr.LoopPositionOffset)
        {
          AddArea(LOOPED_POSITION, hdr.LoopPositionOffset - unfixDelta);
        }
        AddArea(END, size);
        if (!CheckHeader())
        {
          return;
        }
        if (CheckSamplesOffsets())
        {
          const std::size_t sample1Addr = *data.SubView(sample1AddrOffset).As<le_uint16_t>() - unfixDelta;
          AddArea(SAMPLES, sample1Addr);
        }
        if (CheckOrnamentsOffsets())
        {
          const std::size_t ornament1Addr = *data.SubView(ornament1AddrOffset).As<le_uint16_t>() - unfixDelta;
          AddArea(ORNAMENTS, ornament1Addr);
        }
      }

      bool CheckHeader() const
      {
        return sizeof(RawHeader) == GetAreaSize(HEADER) && Undefined == GetAreaSize(END);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES);
        return size != Undefined && size >= sizeof(RawSample);
      }

      bool CheckOrnaments() const
      {
        if (GetAreaAddress(ORNAMENTS_OFFSETS) == Undefined)
        {
          return true;
        }
        else if (!CheckOrnamentsOffsets())
        {
          return false;
        }
        const std::size_t ornamentsSize = GetAreaSize(ORNAMENTS);
        return ornamentsSize != Undefined && ornamentsSize >= sizeof(RawOrnament);
      }

      bool CheckPatterns() const
      {
        const std::size_t size = GetAreaSize(PATTERNS_OFFSETS);
        return size != Undefined && size >= sizeof(uint16_t);
      }

      bool CheckPositions() const
      {
        if (GetAreaAddress(LOOPED_POSITION) != Undefined)
        {
          const std::size_t baseSize = GetAreaSize(POSITIONS);
          if (baseSize == Undefined || 0 != baseSize % sizeof(RawPosEntry))
          {
            return false;
          }
          const std::size_t restSize = GetAreaSize(LOOPED_POSITION);
          return restSize != Undefined && restSize >= sizeof(RawPosEntry) + sizeof(RawPosEntry::Channel);
        }
        else
        {
          const std::size_t baseSize = GetAreaSize(POSITIONS);
          return baseSize != Undefined && baseSize >= sizeof(RawPosEntry) + sizeof(RawPosEntry::Channel);
        }
      }

    private:
      bool CheckSamplesOffsets() const
      {
        const std::size_t size = GetAreaSize(SAMPLES_OFFSETS);
        return size != Undefined && size >= sizeof(uint16_t);
      }

      bool CheckOrnamentsOffsets() const
      {
        const std::size_t size = GetAreaSize(ORNAMENTS_OFFSETS);
        return size != Undefined && size >= sizeof(uint16_t);
      }
    };

    bool FastCheck(const Areas& areas)
    {
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
      if (!areas.CheckPatterns())
      {
        return false;
      }
      if (!areas.CheckPositions())
      {
        return false;
      }
      return true;
    }

    Binary::View MakeContainer(Binary::View rawData)
    {
      return rawData.SubView(0, MAX_MODULE_SIZE);
    }

    bool FastCheck(Binary::View data)
    {
      const auto* hdr = data.As<RawHeader>();
      if (nullptr == hdr)
      {
        return false;
      }
      const std::size_t minSamplesAddr = offsetof(RawHeader, LoopPositionOffset);
      const std::size_t samplesAddr = hdr->SamplesOffset;
      const std::size_t ornamentAddr = hdr->OrnamentsOffset;
      const std::size_t patternAddr = hdr->PatternsOffset;
      const std::size_t positionAddr = hdr->PositionsOffset;
      const std::size_t loopPosAddr = hdr->LoopPositionOffset;
      if (samplesAddr < minSamplesAddr || samplesAddr >= ornamentAddr || ornamentAddr > patternAddr
          || patternAddr >= positionAddr || positionAddr > loopPosAddr)
      {
        return false;
      }
      const std::size_t compileAddr = samplesAddr - minSamplesAddr;
      const Areas areas(data, compileAddr, data.Size());
      return FastCheck(areas);
    }

    const Char DESCRIPTION[] = "SQ-Tracker Compiled";
    // TODO: size may be <256
    const auto FORMAT =
        "?01-30"        // uint16_t Size;
        "?00|60-fb"     // uint16_t SamplesOffset;
        "?00|60-fb"     // uint16_t OrnamentsOffset;
        "?00|60-fb"     // uint16_t PatternsOffset;
        "?01-2d|61-ff"  // uint16_t PositionsOffset;
        "?01-30|61-ff"  // uint16_t LoopPositionOffset;
        // sample1 offset
        "?00|60-fb"
        // pattern1 offset minimal
        "?00-01|60-fc"
        ""_sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_MODULE_SIZE))
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

    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& rawData, Builder& target)
    {
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
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, statistic);
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

        Require(format.GetSize() >= MIN_MODULE_SIZE);
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
  }  // namespace SQTracker

  Decoder::Ptr CreateSQTrackerDecoder()
  {
    return MakePtr<SQTracker::Decoder>();
  }
}  // namespace Formats::Chiptune
