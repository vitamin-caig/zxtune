/**
 *
 * @file
 *
 * @brief  SoundTracker v3.x support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/soundtracker_detail.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/metainfo.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/optimize.h>
#include <tools/iterators.h>
#include <tools/range_checker.h>
// std includes
#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace SoundTracker3
  {
    const Debug::Stream Dbg("Formats::Chiptune::SoundTracker3");

    const Char PROGRAM[] = "Sound Tracker v3.x";

    using namespace SoundTracker;

    const std::size_t MIN_SIZE = 0x180;
    const std::size_t MAX_SIZE = 0x1800;

    /*
      Typical module structure

      Header            13
      Identifier        optional - used to store data from player
      Samples           2*130=260 .. 16*130=2080
      PositionsInfo     1+1*2=3 .. 1+256*2=513
      SamplesInfo       1+2*2=5 .. 1+16*2=33
      Ornaments         2*32=64 .. 16*32=512
      OrnamentsInfo     1+2*2=5 .. 1+16*2=33
      Patterns data     ~16 .. ~0x3000
      PatternsInfo      6 .. 252 (multiply by 6)
    */

    struct RawHeader
    {
      uint8_t Tempo;
      le_uint16_t PositionsOffset;
      le_uint16_t SamplesOffset;
      le_uint16_t OrnamentsOffset;
      le_uint16_t PatternsOffset;
    };

    const uint8_t ID[] = {'K', 'S', 'A', ' ', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', ' ', 'C',
                          'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '};

    struct RawId
    {
      uint8_t Id[sizeof(ID)];
      std::array<char, 27> Title;

      bool Check() const
      {
        return 0 == std::memcmp(Id, ID, sizeof(Id));
      }
    };

    struct RawPositions
    {
      uint8_t Length;
      struct PosEntry
      {
        int8_t Transposition;
        uint8_t PatternOffset;  //*6
      };
      PosEntry Data[1];
    };

    struct RawPattern
    {
      std::array<le_uint16_t, 3> Offsets;
    };

    struct RawSamples
    {
      uint8_t Count;
      le_uint16_t Offsets[1];
    };

    struct RawOrnaments
    {
      uint8_t Count;
      le_uint16_t Offsets[1];
    };

    struct RawOrnament
    {
      std::array<int8_t, ORNAMENT_SIZE> Data;
    };

    struct RawSample
    {
      uint8_t Loop;
      uint8_t LoopLimit;

      // NxxTAAAA
      // nnnnnnnn
      // N - noise mask
      // T - tone mask
      // A - Level
      // n - noise
      struct Line
      {
        le_int16_t Vibrato;
        uint8_t LevelAndMask;
        uint8_t Noise;

        uint_t GetLevel() const
        {
          return LevelAndMask & 15;
        }

        int_t GetEffect() const
        {
          return Vibrato;
        }

        uint_t GetNoise() const
        {
          return Noise;
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelAndMask & 128);
        }

        bool GetToneMask() const
        {
          return 0 != (LevelAndMask & 16);
        }
      };
      Line Data[SAMPLE_SIZE];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 9, "Invalid layout");
    static_assert(sizeof(RawId) * alignof(RawId) == 55, "Invalid layout");
    static_assert(sizeof(RawPositions) * alignof(RawPositions) == 3, "Invalid layout");
    static_assert(sizeof(RawPattern) * alignof(RawPattern) == 6, "Invalid layout");
    static_assert(sizeof(RawSamples) * alignof(RawSamples) == 3, "Invalid layout");
    static_assert(sizeof(RawOrnaments) * alignof(RawOrnaments) == 3, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 32, "Invalid layout");
    static_assert(sizeof(RawSample) * alignof(RawSample) == 130, "Invalid layout");

    class Format
    {
    public:
      explicit Format(Binary::View data)
        : Data(data)
        , Source(*Data.As<RawHeader>())
        , Id(*PeekObject<RawId>(sizeof(Source)))
        , TotalRanges(RangeChecker::CreateSimple(Data.Size()))
        , FixedRanges(RangeChecker::CreateSimple(Data.Size()))
      {
        AddRange(0, sizeof(Source));
        if (Id.Check())
        {
          AddRange(sizeof(Source), sizeof(Id));
        }
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(PROGRAM);
        if (Id.Check())
        {
          meta.SetTitle(Strings::OptimizeAscii(Id.Title));
        }
      }

      void ParsePositions(Builder& builder) const
      {
        Positions positions;
        for (RangeIterator<const RawPositions::PosEntry*> iter = GetPositions(); iter; ++iter)
        {
          const RawPositions::PosEntry& src = *iter;
          Require(0 == src.PatternOffset % sizeof(RawPattern));
          PositionEntry dst;
          dst.PatternIndex = src.PatternOffset / sizeof(RawPattern);
          dst.Transposition = src.Transposition;
          positions.Lines.push_back(dst);
        }
        Dbg("Positions: {} entries", positions.GetSize());
        builder.SetPositions(std::move(positions));
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
        const auto& table = GetObject<RawSamples>(Source.SamplesOffset);
        const uint_t availSamples = table.Count;
        Require(samples.Maximum() < availSamples);
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const std::size_t samOffset = table.Offsets[samIdx];
          Dbg("Parse sample {} at {}", samIdx, samOffset);
          const auto& src = GetObject<RawSample>(samOffset);
          builder.SetSample(samIdx, ParseSample(src));
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        const auto& table = GetObject<RawOrnaments>(Source.OrnamentsOffset);
        const uint_t availOrnaments = table.Count;
        Require(ornaments.Maximum() < availOrnaments);
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          const std::size_t ornOffset = table.Offsets[ornIdx];
          Dbg("Parse ornament {} at {}", ornIdx, ornOffset);
          const auto& src = GetObject<RawOrnament>(ornOffset);
          builder.SetOrnament(ornIdx, ParseOrnament(src));
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
      template<class T>
      const T* PeekObject(std::size_t offset) const
      {
        return Data.SubView(offset).As<T>();
      }

      RangeIterator<const RawPositions::PosEntry*> GetPositions() const
      {
        const std::size_t offset = Source.PositionsOffset;
        const auto* positions = PeekObject<RawPositions>(offset);
        Require(positions != nullptr);
        const uint_t length = positions->Length;
        AddRange(offset, sizeof(*positions) + (length - 1) * sizeof(RawPositions::PosEntry));
        const RawPositions::PosEntry* const firstEntry = positions->Data;
        const RawPositions::PosEntry* const lastEntry = firstEntry + length;
        return {firstEntry, lastEntry};
      }

      template<class T>
      const T& GetObject(std::size_t offset) const
      {
        const auto* src = PeekObject<T>(offset);
        Require(src != nullptr);
        AddRange(offset, sizeof(T));
        return *src;
      }

      const RawPattern& GetPattern(uint_t idx) const
      {
        const std::size_t patOffset = Source.PatternsOffset + idx * sizeof(RawPattern);
        return GetObject<RawPattern>(patOffset);
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

      void ParsePattern(uint_t patIndex, Builder& builder) const
      {
        const RawPattern& src = GetPattern(patIndex);
        const DataCursors rangesStarts(src);
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
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
            state.Period = (cmd - 0xa1) & 0xff;
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
          res.EnvelopeMask = line.GetToneMask();
          res.Effect = line.GetEffect();
        }
        dst.Loop = std::min<uint_t>(src.Loop, SAMPLE_SIZE);
        dst.LoopLimit = std::min<uint_t>(src.Loop + src.LoopLimit, SAMPLE_SIZE);
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
      const RawId& Id;
      const RangeChecker::Ptr TotalRanges;
      const RangeChecker::Ptr FixedRanges;
    };

    enum AreaTypes
    {
      HEADER,
      IDENTIFIER,
      SAMPLES,
      POSITIONS_INFO,
      SAMPLES_INFO,
      ORNAMENTS,
      ORNAMENTS_INFO,
      PATTERNS_INFO,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        const std::size_t idOffset = sizeof(header);
        if (idOffset + sizeof(RawId) <= size)
        {
          const auto* const id = safe_ptr_cast<const RawId*>(&header + 1);
          if (id->Check())
          {
            AddArea(IDENTIFIER, sizeof(header));
          }
        }
        AddArea(POSITIONS_INFO, header.PositionsOffset);
        AddArea(SAMPLES_INFO, header.SamplesOffset);
        AddArea(ORNAMENTS_INFO, header.OrnamentsOffset);
        AddArea(PATTERNS_INFO, header.PatternsOffset);
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return sizeof(RawHeader) <= GetAreaSize(HEADER) && Undefined == GetAreaSize(END);
      }

      bool CheckPositions(uint_t count) const
      {
        if (!Math::InRange<uint_t>(count, 1, MAX_POSITIONS_COUNT))
        {
          return false;
        }
        const std::size_t realSize = GetAreaSize(POSITIONS_INFO);
        const std::size_t requiredSize = sizeof(RawPositions) + (count - 1) * sizeof(RawPositions::PosEntry);
        return realSize != Undefined && realSize >= requiredSize;
      }

      bool CheckSamples(uint_t count) const
      {
        if (!Math::InRange<uint_t>(count, 1, MAX_SAMPLES_COUNT))
        {
          return false;
        }
        {
          const std::size_t realSize = GetAreaSize(SAMPLES_INFO);
          const std::size_t requiredSize = sizeof(RawSamples) + (count - 1) * sizeof(uint16_t);
          if (realSize == Undefined || realSize < requiredSize)
          {
            return false;
          }
        }
        {
          const std::size_t realSize = GetAreaSize(SAMPLES);
          const std::size_t requiredSize = count * sizeof(RawSample);
          return realSize != Undefined && realSize >= requiredSize;
        }
      }

      bool CheckOrnaments(uint_t count) const
      {
        if (!Math::InRange<uint_t>(count, 1, MAX_ORNAMENTS_COUNT))
        {
          return false;
        }
        {
          const std::size_t realSize = GetAreaSize(ORNAMENTS_INFO);
          const std::size_t requiredSize = sizeof(RawOrnaments) + (count - 1) * sizeof(uint16_t);
          if (realSize == Undefined || realSize < requiredSize)
          {
            return false;
          }
        }
        {
          const std::size_t realSize = GetAreaSize(ORNAMENTS);
          const std::size_t requiredSize = count * sizeof(RawOrnament);
          return realSize != Undefined && realSize >= requiredSize;
        }
      }
    };

    Binary::View MakeContainer(Binary::View rawData)
    {
      return rawData.SubView(0, MAX_SIZE);
    }

    bool IsInvalidPosEntry(const RawPositions::PosEntry& entry)
    {
      return 0 != entry.PatternOffset % sizeof(RawPattern);
    }

    bool AreSequenced(le_uint16_t lh, le_uint16_t rh, std::size_t multiply)
    {
      const std::size_t lhNorm = lh;
      const std::size_t rhNorm = rh;
      if (lhNorm > rhNorm)
      {
        return false;
      }
      else if (lhNorm < rhNorm)
      {
        return 0 == (rhNorm - lhNorm) % multiply;
      }
      return true;
    }

    bool FastCheck(Binary::View data)
    {
      const auto* hdr = data.As<RawHeader>();
      if (nullptr == hdr)
      {
        return false;
      }
      Areas areas(*hdr, data.Size());
      const auto* samples = data.SubView(areas.GetAreaAddress(SAMPLES_INFO)).As<RawSamples>();
      const auto* ornaments = data.SubView(areas.GetAreaAddress(ORNAMENTS_INFO)).As<RawOrnaments>();
      if (!samples || !ornaments)
      {
        return false;
      }
      areas.AddArea(SAMPLES, samples->Offsets[0]);
      areas.AddArea(ORNAMENTS, ornaments->Offsets[0]);

      if (!areas.CheckHeader())
      {
        return false;
      }
      if (!areas.CheckSamples(samples->Count))
      {
        return false;
      }
      if (!areas.CheckOrnaments(ornaments->Count))
      {
        return false;
      }
      if (samples->Offsets + samples->Count
          != std::adjacent_find(samples->Offsets, samples->Offsets + samples->Count,
                                [](uint16_t lh, uint16_t rh) { return !AreSequenced(lh, rh, sizeof(RawSample)); }))
      {
        return false;
      }
      if (ornaments->Offsets + ornaments->Count
          != std::adjacent_find(ornaments->Offsets, ornaments->Offsets + ornaments->Count,
                                [](uint16_t lh, uint16_t rh) { return !AreSequenced(lh, rh, sizeof(RawOrnament)); }))
      {
        return false;
      }
      if (const auto* positions = data.SubView(areas.GetAreaAddress(POSITIONS_INFO)).As<RawPositions>())
      {
        const uint_t count = positions->Length;
        if (!areas.CheckPositions(count))
        {
          return false;
        }
        if (positions->Data + count != std::find_if(positions->Data, positions->Data + count, &IsInvalidPosEntry))
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

    const Char DESCRIPTION[] = "Sound Tracker v3.x Compiled";
    const auto FORMAT =
        "03-0f"   // uint8_t Tempo; 1..15
        "?01-08"  // uint16_t PositionsOffset;
        "?01-08"  // uint16_t SamplesOffset;
        "?01-0a"  // uint16_t OrnamentsOffset;
        "?02-16"  // uint16_t PatternsOffset;
        ""sv;

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
        return SoundTracker::Ver3::Parse(rawData, stub);
      }

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const override
      {
        return SoundTracker::Ver3::Parse(data, target);
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace SoundTracker3

  namespace SoundTracker::Ver3
  {
    Decoder::Ptr CreateDecoder()
    {
      return MakePtr<SoundTracker3::Decoder>();
    }

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      using namespace SoundTracker3;

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

    Binary::Container::Ptr InsertMetainformation(const Binary::Container& rawData, Binary::View info)
    {
      using namespace SoundTracker3;
      StatisticCollectingBuilder statistic(GetStubBuilder());
      if (const auto parsed = Parse(rawData, statistic))
      {
        const auto parsedData = MakeContainer(*parsed);
        const auto& header = *parsedData.As<RawHeader>();
        const std::size_t headerSize = sizeof(header);
        const std::size_t infoSize = info.Size();
        const auto patch = PatchedDataBuilder::Create(*parsed);
        const auto* id = parsedData.SubView(headerSize).As<RawId>();
        if (id && id->Check())
        {
          patch->OverwriteData(headerSize, info);
        }
        else
        {
          patch->InsertData(headerSize, info);
          const auto delta = static_cast<int_t>(infoSize);
          patch->FixLEWord(offsetof(RawHeader, PositionsOffset), delta);
          patch->FixLEWord(offsetof(RawHeader, SamplesOffset), delta);
          patch->FixLEWord(offsetof(RawHeader, OrnamentsOffset), delta);
          patch->FixLEWord(offsetof(RawHeader, PatternsOffset), delta);
          const std::size_t patternsStart = header.PatternsOffset;
          const Indices& usedPatterns = statistic.GetUsedPatterns();
          for (Indices::Iterator it = usedPatterns.Items(); it; ++it)
          {
            const std::size_t patOffsets = patternsStart + *it * sizeof(RawPattern);
            patch->FixLEWord(patOffsets + 0, delta);
            patch->FixLEWord(patOffsets + 2, delta);
            patch->FixLEWord(patOffsets + 4, delta);
          }
          // fix all samples and ornaments
          const std::size_t ornamentsStart = header.OrnamentsOffset;
          const auto& orns = *parsedData.SubView(ornamentsStart).As<RawOrnaments>();
          for (uint_t idx = 0; idx != orns.Count; ++idx)
          {
            const std::size_t ornOffset = ornamentsStart + offsetof(RawOrnaments, Offsets) + idx * sizeof(uint16_t);
            patch->FixLEWord(ornOffset, delta);
          }
          const std::size_t samplesStart = header.SamplesOffset;
          const auto& sams = *parsedData.SubView(samplesStart).As<RawSamples>();
          for (uint_t idx = 0; idx != sams.Count; ++idx)
          {
            const std::size_t samOffset = samplesStart + offsetof(RawSamples, Offsets) + idx * sizeof(uint16_t);
            patch->FixLEWord(samOffset, delta);
          }
        }
        return patch->GetResult();
      }
      return {};
    }
  }  // namespace SoundTracker::Ver3

  Formats::Chiptune::Decoder::Ptr CreateSoundTracker3Decoder()
  {
    return SoundTracker::Ver3::CreateDecoder();
  }
}  // namespace Formats::Chiptune
