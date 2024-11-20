/**
 *
 * @file
 *
 * @brief  GlobalTracker support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/globaltracker.h"

#include "formats/chiptune/container.h"
#include "formats/chiptune/metainfo.h"

#include "binary/format_factories.h"
#include "debug/log.h"
#include "math/numeric.h"
#include "strings/format.h"
#include "strings/optimize.h"
#include "tools/indices.h"
#include "tools/iterators.h"
#include "tools/range_checker.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace GlobalTracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::GlobalTracker");

    constexpr auto EDITOR = "Global Tracker v1.{}"sv;

    const std::size_t MIN_SIZE = 1500;
    const std::size_t MAX_SIZE = 0x2800;
    const std::size_t MAX_SAMPLES_COUNT = 15;
    const std::size_t MAX_ORNAMENTS_COUNT = 16;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MIN_PATTERN_SIZE = 4;
    const std::size_t MAX_PATTERN_SIZE = 64;

    /*
      Typical module structure

      Header
      Patterns
      Samples
      Ornaments
    */

    struct RawPattern
    {
      std::array<le_uint16_t, 3> Offsets;
    };

    struct RawHeader
    {
      uint8_t Tempo;
      uint8_t ID[3];
      uint8_t Version;
      le_uint16_t Address;
      std::array<char, 32> Title;
      std::array<le_uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
      std::array<le_uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
      std::array<RawPattern, MAX_PATTERNS_COUNT> Patterns;
      uint8_t Length;
      uint8_t Loop;
      uint8_t Positions[1];
    };

    struct RawObject
    {
      uint8_t Loop;
      uint8_t Size;

      uint_t GetSize() const
      {
        return Size;
      }
    };

    struct RawOrnament : RawObject
    {
      using Line = int8_t;

      Line GetLine(uint_t idx) const
      {
        const auto* const src = safe_ptr_cast<const int8_t*>(this + 1);
        auto offset = static_cast<uint8_t>(idx * sizeof(Line));
        return src[offset];
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }
    };

    struct RawSample : RawObject
    {
      struct Line
      {
        // aaaaaaaa
        // ENTnnnnn
        // vvvvvvvv
        // vvvvvvvv

        // a - level
        // T - tone mask
        // N - noise mask
        // E - env mask
        // n - noise
        // v - signed vibrato
        uint8_t Level;
        uint8_t NoiseAndFlag;
        le_uint16_t Vibrato;

        uint_t GetLevel() const
        {
          return Level;
        }

        bool GetToneMask() const
        {
          return 0 != (NoiseAndFlag & 32);
        }

        bool GetNoiseMask() const
        {
          return 0 != (NoiseAndFlag & 64);
        }

        bool GetEnvelopeMask() const
        {
          return 0 != (NoiseAndFlag & 128);
        }

        uint_t GetNoise() const
        {
          return NoiseAndFlag & 31;
        }

        uint_t GetVibrato() const
        {
          return Vibrato;
        }
      };

      Line GetLine(uint_t idx) const
      {
        static_assert(0 == (sizeof(Line) & (sizeof(Line) - 1)), "Invalid layout");
        const uint_t maxLines = 256 / sizeof(Line);
        const Line* const src = safe_ptr_cast<const Line*>(this + 1);
        return src[idx % maxLines];
      }

      uint_t GetSize() const
      {
        Require(0 == Size % sizeof(Line));
        return (Size ? Size : 256) / sizeof(Line);
      }

      uint_t GetLoop() const
      {
        Require(0 == Loop % sizeof(Line));
        return Loop / sizeof(Line);
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 296, "Invalid layout");
    static_assert(sizeof(RawPattern) * alignof(RawPattern) == 6, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 2, "Invalid layout");
    static_assert(sizeof(RawSample) * alignof(RawSample) == 2, "Invalid layout");
    static_assert(sizeof(RawSample::Line) * alignof(RawSample::Line) == 4, "Invalid layout");

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
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
      void SetEnvelope(uint_t /*type*/, uint_t /*value*/) override {}
      void SetNoEnvelope() override {}
      void SetVolume(uint_t /*vol*/) override {}
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

      void SetInitialTempo(uint_t tempo) override
      {
        return Delegate.SetInitialTempo(tempo);
      }

      void SetSample(uint_t index, Sample sample) override
      {
        assert(0 == index || UsedSamples.Contain(index));
        if (IsSampleSounds(sample))
        {
          NonEmptySamples = true;
        }
        return Delegate.SetSample(index, std::move(sample));
      }

      void SetOrnament(uint_t index, Ornament ornament) override
      {
        assert(0 == index || UsedOrnaments.Contain(index));
        return Delegate.SetOrnament(index, std::move(ornament));
      }

      void SetPositions(Positions positions) override
      {
        UsedPatterns.Assign(positions.Lines.begin(), positions.Lines.end());
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
        UsedOrnaments.Insert(ornament);
        return Delegate.SetOrnament(ornament);
      }

      void SetEnvelope(uint_t type, uint_t value) override
      {
        return Delegate.SetEnvelope(type, value);
      }

      void SetNoEnvelope() override
      {
        return Delegate.SetNoEnvelope();
      }

      void SetVolume(uint_t vol) override
      {
        return Delegate.SetVolume(vol);
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
                           [](const auto& line) { return (!line.ToneMask && line.Level) || !line.NoiseMask; });
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

    bool IsInvalidPosEntry(uint8_t entry)
    {
      return entry % sizeof(RawPattern) != 0
             || !Math::InRange<uint_t>(entry / sizeof(RawPattern) + 1, 1, MAX_PATTERNS_COUNT);
    }

    class Format
    {
    public:
      explicit Format(Binary::View data)
        : Data(data)
        , Ranges(Data.Size())
        , Source(*Data.As<RawHeader>())
        , HeaderSize(sizeof(Source) - 1 + Source.Length)
        , UnfixDelta(Source.Address)
      {
        Ranges.AddService(0, sizeof(Source) - sizeof(Source.Positions));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Strings::Format(EDITOR, Source.Version & 15));
        meta.SetTitle(Strings::OptimizeAscii(Source.Title));
      }

      void ParsePositions(Builder& builder) const
      {
        const auto* const begin = safe_ptr_cast<const uint8_t*>(&Source);
        const uint8_t* const posStart = Source.Positions;
        const uint8_t* const posEnd = posStart + Source.Length;
        Ranges.AddService(posStart - begin, posEnd - posStart);
        Require(posEnd == std::find_if(posStart, posEnd, &IsInvalidPosEntry));
        Positions positions;
        positions.Lines.resize(posEnd - posStart);
        std::transform(posStart, posEnd, positions.Lines.begin(), [](auto b) { return b / sizeof(RawPattern); });
        positions.Loop = std::min<uint_t>(Source.Loop, positions.Lines.size());
        Dbg("Positions: {} entries, loop to {}", positions.GetSize(), positions.GetLoop());
        builder.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: {} to parse", pats.Count());
        bool hasValidPatterns = false;
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          if (ParsePattern(patIndex, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        bool hasValidSamples = false;
        bool hasPartialSamples = false;
        Dbg("Samples: {} to parse", samples.Count());
        const std::size_t minOffset = HeaderSize;
        const std::size_t maxOffset = Data.Size();
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const std::size_t samOffset = GetSampleOffset(samIdx);
          Require(Math::InRange(samOffset, minOffset, maxOffset));
          const std::size_t availSize = maxOffset - samOffset;
          const auto* const src = PeekObject<RawSample>(samOffset);
          Require(src != nullptr);
          Sample result;
          const std::size_t usedSize = src->GetUsedSize();
          if (usedSize <= availSize)
          {
            Dbg("Parse sample {}", samIdx);
            Ranges.Add(samOffset, usedSize);
            ParseSample(*src, src->GetSize(), result);
            hasValidSamples = true;
          }
          else
          {
            Dbg("Parse partial sample {}", samIdx);
            Ranges.Add(samOffset, availSize);
            const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawSample::Line);
            ParseSample(*src, availLines, result);
            hasPartialSamples = true;
          }
          builder.SetSample(samIdx, std::move(result));
        }
        Require(hasValidSamples || hasPartialSamples);
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: {} to parse", ornaments.Count());
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Ornament result;
          if (const std::size_t ornOffset = GetOrnamentOffset(ornIdx))
          {
            Require(ornOffset >= HeaderSize);
            const std::size_t availSize = Data.Size() - ornOffset;
            if (const auto* src = PeekObject<RawOrnament>(ornOffset))
            {
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
            }
            else
            {
              Dbg("Stub ornament {}", ornIdx);
            }
          }
          else
          {
            Dbg("Parse invalid ornament {}", ornIdx);
            result.Lines.push_back(*Data.As<RawOrnament::Line>());
          }
          builder.SetOrnament(ornIdx, std::move(result));
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
      std::size_t GetSampleOffset(uint_t index) const
      {
        if (const uint16_t offset = Source.SamplesOffsets[index])
        {
          Require(offset >= UnfixDelta);
          return offset - UnfixDelta;
        }
        else
        {
          return 0;
        }
      }

      std::size_t GetOrnamentOffset(uint_t index) const
      {
        if (const uint16_t offset = Source.OrnamentsOffsets[index])
        {
          Require(offset >= UnfixDelta);
          return offset - UnfixDelta;
        }
        else
        {
          return 0;
        }
      }

      template<class T>
      const T* PeekObject(std::size_t offset) const
      {
        return Data.SubView(offset).As<T>();
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const auto* data = PeekObject<uint8_t>(offset);
        Require(data != nullptr);
        return *data;
      }

      struct DataCursors : public std::array<std::size_t, 3>
      {
        DataCursors(const RawPattern& src, uint_t minOffset, uint_t unfixDelta)
        {
          for (std::size_t idx = 0; idx != size(); ++idx)
          {
            const std::size_t offset = src.Offsets[idx];
            Require(offset >= minOffset + unfixDelta);
            at(idx) = offset - unfixDelta;
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

      bool ParsePattern(uint_t patIndex, Builder& builder) const
      {
        const RawPattern& src = Source.Patterns[patIndex];
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        const DataCursors rangesStarts(src, HeaderSize, UnfixDelta);
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
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          const ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter)
          {
            continue;
          }
          if (state.Offset >= Data.Size() || (0 == chan && 0xff == PeekByte(state.Offset)))
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
          if (state.Counter)
          {
            --state.Counter;
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(state, builder);
          state.Counter = state.Period;
        }
      }

      void ParseChannel(ParserState::ChannelState& state, Builder& builder) const
      {
        state.Period = 0;
        while (state.Offset < Data.Size())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd <= 0x5f)  // set note
          {
            builder.SetNote(cmd);
            break;
          }
          else if (cmd <= 0x6f)  // set sample
          {
            builder.SetSample(cmd - 0x60);
          }
          else if (cmd <= 0x7f)  // set ornament
          {
            builder.SetOrnament(cmd - 0x70);
            if (NewVersion())
            {
              builder.SetNoEnvelope();
            }
          }
          else if (cmd <= 0xbf)
          {
            state.Period = cmd - 0x80;
          }
          else if (cmd <= 0xcf)
          {
            const uint_t envType = cmd - 0xc0;
            const uint_t envTone = PeekByte(state.Offset++);
            builder.SetEnvelope(envType, envTone);
          }
          else if (cmd <= 0xdf)
          {
            break;
          }
          else if (cmd == 0xe0)
          {
            builder.SetRest();
            if (NewVersion())
            {
              break;
            }
          }
          else if (cmd <= 0xef)
          {
            const uint_t volume = 15 - (cmd - 0xe0);
            builder.SetVolume(volume);
          }
        }
      }

      bool NewVersion() const
      {
        return Source.Version != 0x10;
      }

      static void ParseSample(const RawSample& src, uint_t size, Sample& dst)
      {
        dst.Lines.reserve(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          dst.Lines.emplace_back(ParseSampleLine(line));
        }
        dst.Loop = std::min<int_t>(src.GetLoop(), size);
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& line)
      {
        Sample::Line res;
        res.Level = line.GetLevel();
        res.Noise = line.GetNoise();
        res.ToneMask = line.GetToneMask();
        res.NoiseMask = line.GetNoiseMask();
        res.EnvelopeMask = line.GetEnvelopeMask();
        res.Vibrato = line.GetVibrato();
        return res;
      }

      static void ParseOrnament(const RawOrnament& src, uint_t size, Ornament& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          dst.Lines[idx] = src.GetLine(idx);
        }
        dst.Loop = std::min<int_t>(src.Loop, size);
      }

    private:
      const Binary::View Data;
      RangesMap Ranges;
      const RawHeader& Source;
      const std::size_t HeaderSize;
      const uint_t UnfixDelta;
    };

    Binary::View MakeContainer(Binary::View rawData)
    {
      return rawData.SubView(0, MAX_SIZE);
    }

    std::size_t GetHeaderSize(Binary::View data)
    {
      if (const auto* hdr = data.As<RawHeader>())
      {
        const std::size_t hdrSize = sizeof(*hdr) - 1 + hdr->Length;
        if (hdrSize >= data.Size())
        {
          return 0;
        }
        const uint8_t* const posStart = hdr->Positions;
        const uint8_t* const posEnd = posStart + hdr->Length;
        if (posEnd != std::find_if(posStart, posEnd, &IsInvalidPosEntry))
        {
          return 0;
        }
        return hdrSize;
      }
      return 0;
    }

    enum AreaTypes
    {
      HEADER,
      PATTERNS,
      SAMPLES,
      ORNAMENTS,
      END,
    };

    template<class It>
    std::size_t GetStart(It begin, It end, std::size_t start)
    {
      std::vector<std::size_t> offsets{begin, end};
      std::sort(offsets.begin(), offsets.end());
      auto it = offsets.begin();
      if (it != offsets.end() && *it == 0)
      {
        ++it;
      }
      return it != offsets.end() && *it >= start ? *it - start : 0;
    }

    struct Areas : public AreaController
    {
      Areas(const RawHeader& hdr, std::size_t size)
        : StartAddr(hdr.Address)
      {
        AddArea(HEADER, 0);
        AddArea(PATTERNS, GetStart(hdr.Patterns.front().Offsets.begin(), hdr.Patterns.back().Offsets.end(), StartAddr));
        AddArea(SAMPLES, GetStart(hdr.SamplesOffsets.begin(), hdr.SamplesOffsets.end(), StartAddr));
        AddArea(ORNAMENTS, GetStart(hdr.OrnamentsOffsets.begin(), hdr.OrnamentsOffsets.end(), StartAddr));
        AddArea(END, size);
      }

      bool CheckHeader(std::size_t headerSize) const
      {
        return headerSize + StartAddr < 0x10000 && GetAreaSize(HEADER) >= headerSize && Undefined == GetAreaSize(END);
      }

      bool CheckPatterns() const
      {
        return GetAreaSize(PATTERNS) != 0;
      }

      bool CheckSamples() const
      {
        return GetAreaSize(SAMPLES) != 0;
      }

      bool CheckOrnaments() const
      {
        return GetAreaSize(ORNAMENTS) != 0;
      }

    private:
      const std::size_t StartAddr;
    };

    bool FastCheck(Binary::View data)
    {
      const std::size_t hdrSize = GetHeaderSize(data);
      if (!Math::InRange<std::size_t>(hdrSize, sizeof(RawHeader), data.Size()))
      {
        return false;
      }
      const auto& hdr = *data.As<RawHeader>();
      const Areas areas(hdr, data.Size());
      if (!areas.CheckHeader(hdrSize))
      {
        return false;
      }
      if (!areas.CheckPatterns())
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

    const auto DESCRIPTION = "Global Tracker v1.x"sv;
    const auto FORMAT =
        "03-0f"     // uint8_t Tempo;
        "???"       // uint8_t ID[3];
        "10-12"     // uint8_t Version; who knows?
        "??"        // uint16_t Address;
        "?{32}"     // char Title[32];
        "?{30}"     // std::array<uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
        "?{32}"     // std::array<uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
        "?{192}"    // std::array<RawPattern, MAX_PATTERNS_COUNT> Patterns;
        "01-ff"     // uint8_t Length;
        "00-fe"     // uint8_t Loop;
        "*6&00-ba"  // uint8_t Positions[1];
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {}

      StringView GetDescription() const override
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
      static StubBuilder stub;
      return stub;
    }
  }  // namespace GlobalTracker

  Decoder::Ptr CreateGlobalTrackerDecoder()
  {
    return MakePtr<GlobalTracker::Decoder>();
  }
}  // namespace Formats::Chiptune
