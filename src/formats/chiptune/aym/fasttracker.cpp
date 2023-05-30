/**
 *
 * @file
 *
 * @brief  FastTracker support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/fasttracker.h"
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
#include <strings/optimize.h>
// std includes
#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace FastTracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::FastTracker");

    const Char PROGRAM[] = "Fast Tracker v1.x";

    const std::size_t MIN_MODULE_SIZE = 256;
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

    const uint8_t MODULE_ID[] = {'M', 'o', 'd', 'u', 'l', 'e', ':', ' '};

    enum NoteTableCode : uint8_t
    {
      PROTRACKER2 = ';',
      SOUNDTRACKER = 0x01,
      FASTTRACKER = 0x02
    };

    struct RawId
    {
      uint8_t Identifier[8];  //"Module: "
      std::array<char, 42> Title;
      uint8_t NoteTableSign;  //";"
      std::array<char, 18> Editor;

      bool HasTitle() const
      {
        static_assert(sizeof(MODULE_ID) == sizeof(Identifier), "Invalid layout");
        return 0 == std::memcmp(Identifier, MODULE_ID, sizeof(Identifier));
      }

      bool HasProgram() const
      {
        return NoteTableSign == NoteTableCode::PROTRACKER2 || NoteTableSign == NoteTableCode::SOUNDTRACKER
               || NoteTableSign == NoteTableCode::FASTTRACKER;
      }

      NoteTable GetNoteTable() const
      {
        switch (NoteTableSign)
        {
        case NoteTableCode::SOUNDTRACKER:
          return NoteTable::SOUNDTRACKER;
        case NoteTableCode::FASTTRACKER:
          return NoteTable::FASTTRACKER;
        default:
          return NoteTable::PROTRACKER2;
        }
      }
    };

    struct RawHeader
    {
      RawId Metainfo;
      uint8_t Tempo;
      uint8_t Loop;
      uint8_t Padding1[4];
      le_uint16_t PatternsOffset;
      uint8_t Padding2[5];
      std::array<le_uint16_t, SAMPLES_COUNT> SamplesOffsets;
      std::array<le_uint16_t, ORNAMENTS_COUNT> OrnamentsOffsets;
    };

    struct RawPosition
    {
      uint8_t PatternIndex;
      int8_t Transposition;
    };

    struct RawPattern
    {
      std::array<le_uint16_t, 3> Offsets;
    };

    struct RawObject
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
    };

    struct RawOrnament
    {
      RawObject Obj;
      struct Line
      {
        // nNxooooo
        // OOOOOOOO
        // o - noise offset
        // O - note offset (signed)
        // N - keep note offset
        // n - keep noise offset
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
      };

      std::size_t GetUsedSize() const
      {
        // 8-bit offsets
        return sizeof(RawObject) + Obj.GetSize() * sizeof(Line);
      }

      const Line& GetLine(uint_t idx) const
      {
        return Lines[idx];
      }

      Line Lines[1];
    };

    struct RawSample
    {
      RawObject Obj;
      struct Line
      {
        // KMxnnnnn
        // K - keep noise offset
        // M - noise mask
        // n - noise offset
        uint8_t Noise;
        uint8_t ToneLo;
        // KMxxtttt
        // K - keep tone offset
        // M - tone mask
        uint8_t ToneHi;
        // KMvvaaaa
        // vv = 0x - no slide
        //     10 - +1
        //     11 - -1
        // a - level
        // K - keep envelope offset
        // M - envelope mask
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
      };

      std::size_t GetUsedSize() const
      {
        // 16-bit offsets
        return sizeof(RawObject) + Obj.GetSize() * sizeof(Line);
      }

      const Line& GetLine(uint_t idx) const
      {
        return Lines[idx];
      }

      Line Lines[1];
    };

    static_assert(sizeof(RawId) * alignof(RawId) == 69, "Invalid layout");
    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 212, "Invalid layout");
    static_assert(sizeof(RawPosition) * alignof(RawPosition) == 2, "Invalid layout");
    static_assert(sizeof(RawPattern) * alignof(RawPattern) == 6, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 5, "Invalid layout");
    static_assert(sizeof(RawOrnament::Line) * alignof(RawOrnament::Line) == 2, "Invalid layout");
    static_assert(sizeof(RawSample) * alignof(RawSample) == 8, "Invalid layout");
    static_assert(sizeof(RawSample::Line) * alignof(RawSample::Line) == 5, "Invalid layout");

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      void SetNoteTable(NoteTable /*table*/) override {}
      void SetInitialTempo(uint_t /*tempo*/) override {}
      void SetSample(uint_t /*index*/, Sample /*sample*/) override {}
      void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}
      void SetPositions(Positions /*positions*/) override {}
      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }
      void StartChannel(uint_t /*index*/) override {}
      void SetRest() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetSample(uint_t /*sample*/) override {}
      void SetOrnament(uint_t /*ornament*/) override {}
      void SetVolume(uint_t /*vol*/) override {}
      void SetEnvelope(uint_t /*type*/, uint_t /*tone*/) override {}
      void SetNoEnvelope() override {}
      void SetNoise(uint_t /*val*/) override {}
      void SetSlide(uint_t /*step*/) override {}
      void SetNoteSlide(uint_t /*step*/) override {}
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

      MetaBuilder& GetMetaBuilder() override
      {
        return Delegate.GetMetaBuilder();
      }

      void SetNoteTable(NoteTable table) override
      {
        return Delegate.SetNoteTable(table);
      }

      void SetInitialTempo(uint_t tempo) override
      {
        return Delegate.SetInitialTempo(tempo);
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

      void SetVolume(uint_t vol) override
      {
        return Delegate.SetVolume(vol);
      }

      void SetEnvelope(uint_t type, uint_t tone) override
      {
        return Delegate.SetEnvelope(type, tone);
      }

      void SetNoEnvelope() override
      {
        return Delegate.SetNoEnvelope();
      }

      void SetNoise(uint_t val) override
      {
        return Delegate.SetNoise(val);
      }

      void SetSlide(uint_t step) override
      {
        return Delegate.SetSlide(step);
      }

      void SetNoteSlide(uint_t step) override
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
      Format(Binary::View data, std::size_t baseAddr)
        : Data(data)
        , Ranges(data.Size())
        , Source(*Data.As<RawHeader>())
        , BaseAddr(baseAddr)
      {
        Ranges.AddService(0, sizeof(Source));
        Dbg("Base addr is #{:04x}", BaseAddr);
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        const RawId& in = Source.Metainfo;
        builder.SetNoteTable(in.GetNoteTable());
        MetaBuilder& meta = builder.GetMetaBuilder();
        if (in.HasProgram())
        {
          meta.SetProgram(Strings::OptimizeAscii(in.Editor));
        }
        else
        {
          meta.SetProgram(PROGRAM);
        }
        if (in.HasTitle())
        {
          meta.SetTitle(Strings::OptimizeAscii(in.Title));
        }
      }

      void ParsePositions(Builder& builder) const
      {
        Positions positions;
        const std::size_t positionsStart = sizeof(Source);
        for (uint_t posIdx = 0;; ++posIdx)
        {
          const auto& pos = GetServiceObject<RawPosition>(positionsStart + sizeof(RawPosition) * posIdx);
          const uint_t patIdx = pos.PatternIndex;
          if (patIdx == 0xff)
          {
            break;
          }
          PositionEntry res;
          res.PatternIndex = patIdx;
          res.Transposition = pos.Transposition;
          positions.Lines.push_back(res);
        }
        positions.Loop = Source.Loop;
        Dbg("Positions: {} entries, loop to {}", positions.GetSize(), positions.GetLoop());
        builder.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: {} to parse", pats.Count());
        const std::size_t baseOffset = Source.PatternsOffset;
        const std::size_t minOffset = baseOffset + pats.Maximum() * sizeof(RawPattern);
        bool hasValidPatterns = false;
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          if (ParsePattern(baseOffset, minOffset, patIndex, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Dbg("Samples: {} to parse", samples.Count());
        uint_t nonEmptySamplesCount = 0;
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const std::size_t samOffset = Source.SamplesOffsets[samIdx] - BaseAddr;
          const std::size_t availSize = Data.Size() - samOffset;
          const auto* src = PeekObject<RawSample>(samOffset);
          Require(src != nullptr);
          Require(src->Obj.GetLoopLimit() <= src->Obj.GetSize());
          Require(src->Obj.GetLoop() <= src->Obj.GetLoopLimit());
          const std::size_t usedSize = src->GetUsedSize();
          Require(usedSize <= availSize);
          Dbg("Parse sample {}", samIdx);
          Ranges.AddService(samOffset, usedSize);
          builder.SetSample(samIdx, ParseSample(*src, src->Obj.GetLoopLimit()));
          if (src->Obj.GetLoopLimit() > 1 || !src->GetLine(0).IsEmpty())
          {
            ++nonEmptySamplesCount;
          }
        }
        Require(nonEmptySamplesCount > 0);
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: {} to parse", ornaments.Count());
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          const std::size_t ornOffset = Source.OrnamentsOffsets[ornIdx] - BaseAddr;
          const std::size_t availSize = Data.Size() - ornOffset;
          const auto* src = PeekObject<RawOrnament>(ornOffset);
          Require(src != nullptr);
          Require(src->Obj.GetLoopLimit() <= src->Obj.GetSize());
          Require(src->Obj.GetLoop() <= src->Obj.GetLoopLimit());
          const std::size_t usedSize = src->GetUsedSize();
          Require(usedSize <= availSize);
          Dbg("Parse ornament {}", ornIdx);
          Ranges.AddService(ornOffset, usedSize);
          builder.SetOrnament(ornIdx, ParseOrnament(*src, src->Obj.GetLoopLimit()));
        }
        for (uint_t ornIdx = ornaments.Maximum() + 1; ornIdx < ORNAMENTS_COUNT; ++ornIdx)
        {
          const std::size_t ornOffset = Source.OrnamentsOffsets[ornIdx] - BaseAddr;
          const std::size_t availSize = Data.Size() - ornOffset;
          if (const auto* src = PeekObject<RawOrnament>(ornOffset))
          {
            const std::size_t usedSize = src->GetUsedSize();
            if (usedSize <= availSize)
            {
              Dbg("Stub ornament {}", ornIdx);
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
      const T* PeekObject(std::size_t offset) const
      {
        return Data.SubView(offset).As<T>();
      }

      template<class T>
      const T& GetServiceObject(std::size_t offset) const
      {
        const T* src = PeekObject<T>(offset);
        Require(src != nullptr);
        Ranges.AddService(offset, sizeof(T));
        return *src;
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const auto* data = PeekObject<uint8_t>(offset);
        Require(data != nullptr);
        return *data;
      }

      uint_t PeekLEWord(std::size_t offset) const
      {
        const auto* const data = PeekObject<le_uint16_t>(offset);
        Require(data != nullptr);
        return *data;
      }

      struct DataCursors : public std::array<std::size_t, 3>
      {
        DataCursors(const RawPattern& src, std::size_t baseOffset)
        {
          for (std::size_t chan = 0; chan != src.Offsets.size(); ++chan)
          {
            const std::size_t addr = src.Offsets[chan];
            Require(addr >= baseOffset);
            at(chan) = addr - baseOffset;
          }
        }
      };

      struct ParserState
      {
        struct ChannelState
        {
          std::size_t Offset = 0;
          uint_t Period = 0;
          uint_t Counter = 0;

          ChannelState() = default;

          void Skip(uint_t toSkip)
          {
            Counter -= toSkip;
          }

          static bool CompareByCounter(const ChannelState& lh, const ChannelState& rh)
          {
            return lh.Counter < rh.Counter;
          }
        };

        std::array<ChannelState, 3> Channels;

        explicit ParserState(const DataCursors& src)

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
          for (auto& chan : Channels)
          {
            chan.Skip(toSkip);
          }
        }
      };

      bool ParsePattern(std::size_t baseOffset, std::size_t minOffset, uint_t patIndex, Builder& builder) const
      {
        const auto& pat = GetServiceObject<RawPattern>(baseOffset + patIndex * sizeof(RawPattern));
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        const DataCursors rangesStarts(pat, BaseAddr);
        Require(
            rangesStarts.end()
            == std::find_if(rangesStarts.begin(), rangesStarts.end(), Math::NotInRange(minOffset, Data.Size() - 1)));

        ParserState state(rangesStarts);
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
          patBuilder.StartLine(lineIdx);
          ParseLine(state, patBuilder, builder);
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
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          const ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter)
          {
            continue;
          }
          if (state.Offset >= Data.Size())
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

      void ParseLine(ParserState& src, PatternBuilder& patBuilder, Builder& builder) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter)
          {
            --state.Counter;
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(state, patBuilder, builder);
          state.Counter = state.Period;
        }
      }

      void ParseChannel(ParserState::ChannelState& state, PatternBuilder& patBuilder, Builder& builder) const
      {
        while (state.Offset < Data.Size())
        {
          const uint_t cmd = PeekByte(state.Offset++);
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
            state.Period = 0;
            break;
          }
          else if (cmd <= 0x3e)
          {
            const uint_t type = cmd - 0x30;
            const uint_t tone = PeekLEWord(state.Offset);
            builder.SetEnvelope(type, tone);
            state.Offset += 2;
          }
          else if (cmd == 0x3f)
          {
            builder.SetNoEnvelope();
          }
          else if (cmd <= 0x5f)
          {
            state.Period = cmd - 0x40;
            break;
          }
          else if (cmd <= 0xcb)
          {
            builder.SetNote(cmd - 0x60);
            state.Period = 0;
            break;
          }
          else if (cmd <= 0xec)
          {
            builder.SetOrnament(cmd - 0xcc);
          }
          else if (cmd == 0xed)
          {
            const uint_t step = PeekLEWord(state.Offset);
            builder.SetSlide(step);
            state.Offset += 2;
          }
          else if (cmd == 0xee)
          {
            const uint_t noteSlideStep = PeekByte(state.Offset++);
            builder.SetNoteSlide(noteSlideStep);
          }
          else if (cmd == 0xef)
          {
            const uint_t noise = PeekByte(state.Offset++);
            builder.SetNoise(noise);
          }
          else
          {
            const uint_t tempo = PeekByte(state.Offset++);
            Require(Math::InRange(tempo, TEMPO_MIN, TEMPO_MAX));
            patBuilder.SetTempo(tempo);
          }
        }
      }

      static Sample ParseSample(const RawSample& src, uint_t size)
      {
        Sample dst;
        Require(size <= MAX_SAMPLE_SIZE);
        dst.Lines.reserve(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          dst.Lines.emplace_back(ParseSampleLine(line));
        }
        dst.Loop = std::min<uint_t>(src.Obj.GetLoop(), dst.Lines.size());
        return dst;
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

      static Ornament ParseOrnament(const RawOrnament& src, uint_t size)
      {
        Ornament dst;
        Require(size <= MAX_ORNAMENT_SIZE);
        dst.Lines.reserve(size);
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawOrnament::Line& line = src.GetLine(idx);
          dst.Lines.emplace_back(ParseOrnamentLine(line));
        }
        dst.Loop = std::min<uint_t>(src.Obj.GetLoop(), dst.Lines.size());
        return dst;
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
      const Binary::View Data;
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
      explicit Areas(Binary::View data)
        : BaseAddr(0)
      {
        const auto& header = *data.As<RawHeader>();
        const std::size_t firstPatternOffset = header.PatternsOffset;
        AddArea(HEADER, 0);
        AddArea(POSITIONS, sizeof(header));
        AddArea(PATTERNS, firstPatternOffset);
        AddArea(END, data.Size());
        if (GetAreaSize(PATTERNS) < sizeof(RawPattern))
        {
          return;
        }
        const std::size_t firstOrnamentOffset = header.OrnamentsOffsets[0];
        const std::size_t firstSampleOffset = header.SamplesOffsets[0];
        if (firstSampleOffset >= firstOrnamentOffset)
        {
          return;
        }
        std::size_t patternDataAddr = 0;
        for (std::size_t pos = firstPatternOffset;; pos += sizeof(RawPattern))
        {
          if (const auto* pat = data.SubView(pos).As<RawPattern>())
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
              patternDataAddr = pat->Offsets[0];
            }
          }
          else
          {
            return;
          }
        }
        if (firstSampleOffset > BaseAddr && data.SubView(firstSampleOffset - BaseAddr).As<RawOrnament>())
        {
          AddArea(SAMPLES, firstSampleOffset - BaseAddr);
        }
        if (firstOrnamentOffset > BaseAddr && data.SubView(firstOrnamentOffset - BaseAddr).As<RawOrnament>())
        {
          AddArea(ORNAMENTS, firstOrnamentOffset - BaseAddr);
        }
      }

      bool CheckHeader() const
      {
        return GetAreaSize(HEADER) >= sizeof(RawHeader) && Undefined == GetAreaSize(END);
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

    bool FastCheck(Binary::View data)
    {
      const Areas areas(data);
      return FastCheck(areas);
    }

    Binary::View MakeContainer(Binary::View data)
    {
      return data.SubView(0, MAX_MODULE_SIZE);
    }

    const auto DESCRIPTION = PROGRAM;
    const auto FORMAT =
        "?{8}"                // identifier
        "?{42}"               // title
        "?"                   // semicolon
        "?{18}"               // editor
        "03-ff"               // tempo
        "00-fe"               // loop
        "?{4}"                // padding1
        "?00-02"              // patterns offset
        "?{5}"                // padding2
        "(?03-2c|64-ff){32}"  // samples
        "(?05-2d|66-ff){33}"  // ornaments
        "00-1f?"              // at least one position
        "ff|00-1f"            // next position or end
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
        return Parse(rawData, stub);
      }

    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const auto data = MakeContainer(rawData);
      const Areas areas(data);
      if (!FastCheck(areas))
      {
        return {};
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
  }  // namespace FastTracker

  Decoder::Ptr CreateFastTrackerDecoder()
  {
    return MakePtr<FastTracker::Decoder>();
  }
}  // namespace Formats::Chiptune
