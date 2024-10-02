/**
 *
 * @file
 *
 * @brief  SoundTracker compiled modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/soundtracker_detail.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <iterator.h>
#include <make_ptr.h>
#include <range_checker.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/optimize.h>
// std includes
#include <algorithm>
#include <array>

namespace Formats::Chiptune
{
  namespace SoundTrackerCompiled
  {
    const Debug::Stream Dbg("Formats::Chiptune::SoundTrackerCompiled");

    const Char PROGRAM[] = "Sound Tracker v1.x";

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

    struct RawHeader
    {
      uint8_t Tempo;
      le_uint16_t PositionsOffset;
      le_uint16_t OrnamentsOffset;
      le_uint16_t PatternsOffset;
      std::array<char, 18> Identifier;
      le_uint16_t Size;
    };

    struct RawPositions
    {
      uint8_t Length;
      struct PosEntry
      {
        uint8_t PatternNum;
        int8_t Transposition;
      };
      PosEntry Data[1];
    };

    struct RawPattern
    {
      uint8_t Number;
      std::array<le_uint16_t, 3> Offsets;
    };

    struct RawOrnament
    {
      uint8_t Number;
      std::array<int8_t, ORNAMENT_SIZE> Data;
    };

    struct RawSample
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
      struct Line
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
      };
      Line Data[SAMPLE_SIZE];
      uint8_t Loop;
      uint8_t LoopSize;
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 27, "Invalid layout");
    static_assert(sizeof(RawPositions) * alignof(RawPositions) == 3, "Invalid layout");
    static_assert(sizeof(RawPattern) * alignof(RawPattern) == 7, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 33, "Invalid layout");
    static_assert(sizeof(RawSample) * alignof(RawSample) == 99, "Invalid layout");

    bool IsProgramName(StringView name)
    {
      static const std::array STANDARD_PROGRAMS_PREFIXES = {
          "SONG BY ST COMPIL"sv, "SONG BY MB COMPIL"sv,  "SONG BY ST-COMPIL"sv,
          "SONG BY S.T.COMP"sv,  "SONG ST BY COMPILE"sv, "SOUND TRACKER"sv,
          "S.T.FULL EDITION"sv,  "S.W.COMPILE V2.0"sv,   "STU SONG COMPILER"sv,
      };
      return std::any_of(STANDARD_PROGRAMS_PREFIXES.begin(), STANDARD_PROGRAMS_PREFIXES.end(),
                         [name](auto prefix) { return name.starts_with(prefix); });
    }

    class Format
    {
    public:
      explicit Format(Binary::View data)
        : Data(data)
        , Source(*Data.As<RawHeader>())
        , TotalRanges(RangeChecker::CreateSimple(Data.Size()))
        , FixedRanges(RangeChecker::CreateSimple(Data.Size()))
      {
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        const auto id = MakeStringView(Source.Identifier);
        if (IsProgramName(id))
        {
          meta.SetProgram(Strings::OptimizeAscii(id));
        }
        else
        {
          meta.SetTitle(Strings::OptimizeAscii(id));
          meta.SetProgram(PROGRAM);
        }
      }

      void ParsePositions(Builder& builder) const
      {
        Positions positions;
        for (RangeIterator<const RawPositions::PosEntry*> iter = GetPositions(); iter; ++iter)
        {
          const RawPositions::PosEntry& src = *iter;
          Require(Math::InRange<uint_t>(src.PatternNum, 1, MAX_PATTERNS_COUNT));
          PositionEntry dst;
          dst.PatternIndex = src.PatternNum - 1;
          dst.Transposition = src.Transposition;
          positions.Lines.push_back(dst);
        }
        Dbg("Positions: {} entries", positions.GetSize());
        builder.SetPositions(std::move(positions));
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
          Dbg("Parse pattern {}", patIndex);
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
            Dbg("Fill stub pattern {}", idx);
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
          Dbg("Parse sample {}", samIdx);
          builder.SetSample(samIdx, ParseSample(src));
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
            Dbg("Fill stub sample {}", idx);
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
          Dbg("Parse ornament {}", ornIdx);
          builder.SetOrnament(ornIdx, ParseOrnament(src));
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
            Dbg("Fill stub ornament {}", idx);
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
        const std::size_t offset = Source.PositionsOffset;
        const auto* positions = PeekObject<RawPositions>(offset);
        Require(positions != nullptr);
        const uint_t length = positions->Length + 1;
        AddRange(offset, sizeof(*positions) + (length - 1) * sizeof(RawPositions::PosEntry));
        const RawPositions::PosEntry* const firstEntry = positions->Data;
        const RawPositions::PosEntry* const lastEntry = firstEntry + length;
        return {firstEntry, lastEntry};
      }

      const RawPattern& GetPattern(uint_t index) const
      {
        return GetObject<RawPattern>(index, Source.PatternsOffset);
      }

      const RawSample& GetSample(uint_t index) const
      {
        return GetObject<RawSample>(index, sizeof(Source));
      }

      const RawOrnament& GetOrnament(uint_t index) const
      {
        return GetObject<RawOrnament>(index, Source.OrnamentsOffset);
      }

      template<class T>
      const T& GetObject(uint_t index, std::size_t baseOffset) const
      {
        const std::size_t offset = baseOffset + index * sizeof(T);
        const auto* src = PeekObject<T>(offset);
        Require(src != nullptr);
        AddRange(offset, sizeof(T));
        return *src;
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
        explicit DataCursors(const RawPattern& src)
        {
          std::copy(src.Offsets.begin(), src.Offsets.end(), begin());
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

      void ParsePattern(const RawPattern& src, Builder& builder) const
      {
        PatternBuilder& patBuilder = builder.StartPattern(src.Number - 1);
        const DataCursors rangesStarts(src);

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
            Dbg("Affected ranges {}..{}", start, stop);
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
        while (state.Offset < Data.Size())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd <= 0x5f)  // note
          {
            builder.SetNote(cmd);
            break;
          }
          else if (cmd <= 0x6f)  // sample
          {
            builder.SetSample(cmd - 0x60);
          }
          else if (cmd <= 0x7f)  // ornament
          {
            builder.SetNoEnvelope();
            builder.SetOrnament(cmd - 0x70);
          }
          else if (cmd == 0x80)  // reset
          {
            builder.SetRest();
            break;
          }
          else if (cmd == 0x81)  // empty
          {
            break;
          }
          else if (cmd == 0x82)  // orn 0 without envelope
          {
            builder.SetOrnament(0);
            builder.SetNoEnvelope();
          }
          else if (cmd <= 0x8e)  // orn 0, with envelope
          {
            builder.SetOrnament(0);
            const uint_t envPeriod = PeekByte(state.Offset++);
            builder.SetEnvelope(cmd - 0x80, envPeriod);
          }
          else
          {
            state.Period = (cmd + 0x100 - 0xa1) & 0xff;
          }
        }
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
          res.NoiseMask = line.GetNoiseMask();
          res.EnvelopeMask = line.GetEnvelopeMask();
          res.Effect = line.GetEffect();
        }
        dst.Loop = std::min<uint_t>(src.Loop, SAMPLE_SIZE);
        dst.LoopLimit = std::min<uint_t>(src.Loop + src.LoopSize, SAMPLE_SIZE);
        return dst;
      }

      static Ornament ParseOrnament(const RawOrnament& src)
      {
        Ornament dst;
        dst.Lines.assign(src.Data.begin(), src.Data.end());
        return dst;
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
      const Binary::View Data;
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
        AddArea(POSITIONS, header.PositionsOffset);
        // first ornament can be overlapped by the other structures
        AddArea(ORNAMENTS, header.OrnamentsOffset);
        AddArea(PATTERNS, header.PatternsOffset);
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

    Binary::View MakeContainer(Binary::View rawData)
    {
      return rawData.SubView(0, MAX_SIZE);
    }

    bool FastCheck(Binary::View data)
    {
      const auto* hdr = data.As<RawHeader>();
      if (nullptr == hdr)
      {
        return false;
      }
      const Areas areas(*hdr, data.Size());
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
      if (const auto* positions = data.SubView(areas.GetAreaAddress(POSITIONS)).As<RawPositions>())
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
      return data.Size() >= std::max(areas.GetAreaAddress(ORNAMENTS) + sizeof(RawOrnament),
                                     areas.GetAreaAddress(PATTERNS) + sizeof(RawPattern));
    }

    const Char DESCRIPTION[] = "Sound Tracker v1.x Compiled";
    // Statistic-based format based on 6k+ files
    const auto FORMAT =
        "01-20"   // uint8_t Tempo; 1..50
        "?00-07"  // uint16_t PositionsOffset;
        "?00-07"  // uint16_t OrnamentsOffset;
        "?00-08"  // uint16_t PatternsOffset;
        "?{20}"   // Id+Size
        "00-0f"   // first sample index
        ""sv;

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

    class Decoder : public Formats::Chiptune::SoundTracker::Decoder
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
        return FastCheck(rawData) && Format->Match(rawData);
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

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const override
      {
        return ParseCompiled(data, target);
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace SoundTrackerCompiled

  namespace SoundTracker::Ver1
  {
    Decoder::Ptr CreateCompiledDecoder()
    {
      return MakePtr<SoundTrackerCompiled::Decoder>();
    }
  }  // namespace SoundTracker::Ver1

  Formats::Chiptune::Decoder::Ptr CreateSoundTrackerCompiledDecoder()
  {
    return SoundTracker::Ver1::CreateCompiledDecoder();
  }
}  // namespace Formats::Chiptune
