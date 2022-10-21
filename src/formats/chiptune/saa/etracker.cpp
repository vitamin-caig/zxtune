/**
 *
 * @file
 *
 * @brief  ETracker support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/saa/etracker.h"
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
  namespace ETracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::ETracker");

    const Char DESCRIPTION[] = "E-Tracker v1.x";

    const std::size_t MIN_SIZE = 96;
    const std::size_t MAX_SIZE = 0x8000;
    const std::size_t MAX_POSITIONS_COUNT = 255;
    const std::size_t MIN_PATTERN_SIZE = 1;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_SAMPLES_COUNT = 32;
    const std::size_t MAX_ORNAMENTS_COUNT = 32;
    // const std::size_t MAX_SAMPLE_SIZE = 256;
    // const std::size_t MAX_ORNAMENT_SIZE = 256;

    /*
      Typical module structure

      Header
      Patterns data
      Ornaments
      Samples
      SamplesTable
      OrnamentsTable
      SamplesVolumeDecodeTable
      PositionsTable
      PatternsTable
    */

    struct RawHeader
    {
      le_uint16_t PositionsOffset;
      le_uint16_t PatternsOffset;
      le_uint16_t SamplesOffset;
      le_uint16_t OrnamentsOffset;
      le_uint16_t SamplesVolumeDecodeTable;
      uint8_t Signature[0x14];
    };

    struct RawPattern
    {
      std::array<le_uint16_t, 6> Offsets;
    };

    struct RawSamplesVolumeDecodeTable
    {
      uint8_t Marker;
      uint8_t Table[1];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x1e, "Invalid layout");
    static_assert(sizeof(RawPattern) * alignof(RawPattern) == 12, "Invalid layout");

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
      void SetAttenuation(uint_t /*vol*/) override {}
      void SetSwapSampleChannels(bool /*swapSamples*/) override {}
      void SetEnvelope(uint_t /*value*/) override {}
      void SetNoise(uint_t /*type*/) override {}
    };

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
        , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
        , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
      {}

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
        assert(index == 0 || UsedSamples.Contain(index));
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

      void SetAttenuation(uint_t vol) override
      {
        return Delegate.SetAttenuation(vol);
      }

      void SetSwapSampleChannels(bool swapChannels) override
      {
        return Delegate.SetSwapSampleChannels(swapChannels);
      }

      void SetEnvelope(uint_t value) override
      {
        return Delegate.SetEnvelope(value);
      }

      void SetNoise(uint_t type) override
      {
        return Delegate.SetNoise(type);
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

    struct DecodeTable
    {
      uint_t Marker;
      struct CodeAndLen
      {
        uint_t Code;
        uint_t Len;
      };
      std::vector<CodeAndLen> Lenghts;

      DecodeTable()
        : Marker()
      {}

      uint_t DecodeLen(uint_t code) const
      {
        for (auto len : Lenghts)
        {
          if (len.Code == code)
          {
            return len.Len;
          }
        }
        return 0;
      }
    };

    class Format
    {
    public:
      Format(Binary::View data)
        : Data(data)
        , Ranges(Data.Size())
        , Source(*Data.As<RawHeader>())
      {
        Ranges.AddService(0, sizeof(Source));
        ReadSampleDecodeTable();
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(6);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(DESCRIPTION);
      }

      void ParsePositions(Builder& builder) const
      {
        Positions positions;
        PositionEntry entry;
        for (std::size_t posCursor = Source.PositionsOffset;; ++posCursor)
        {
          Require(positions.GetSize() <= MAX_POSITIONS_COUNT);
          const uint_t val = PeekByte(posCursor);
          if (val == 0xff)
          {
            break;
          }
          else if (val == 0xfe)
          {
            positions.Loop = positions.GetSize();
          }
          else if (val >= 0x60)
          {
            entry.Transposition = val - 0x60;
          }
          else
          {
            Require(0 == val % 3);
            entry.PatternIndex = val / 3;
            positions.Lines.push_back(entry);
          }
        }
        Require(!positions.Lines.empty());
        Dbg("Positions: {} entries, loop to {}", positions.GetSize(), positions.GetLoop());
        builder.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: {} to parse", pats.Count());
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          ParsePattern(patIndex, builder);
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Dbg("Samples: {} to parse", samples.Count());
        const std::size_t samplesTable = Source.SamplesOffset;
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Dbg("Parse sample {}", samIdx);
          const std::size_t samOffset = ReadWord(samplesTable, samIdx);
          builder.SetSample(samIdx, ParseSample(samOffset));
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: {} to parse", ornaments.Count());
        const std::size_t ornamentsTable = Source.OrnamentsOffset;
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Dbg("Parse ornament {}", ornIdx);
          const std::size_t ornOffset = ReadWord(ornamentsTable, ornIdx);
          builder.SetOrnament(ornIdx, ParseOrnament(ornOffset));
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

      const RawPattern& GetPattern(uint_t patIdx) const
      {
        const std::size_t patOffset = Source.PatternsOffset + patIdx * sizeof(RawPattern);
        Ranges.AddService(patOffset, sizeof(RawPattern));
        return *PeekObject<RawPattern>(patOffset);
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const auto* data = PeekObject<uint8_t>(offset);
        Require(data != nullptr);
        return *data;
      }

      uint16_t ReadWord(std::size_t base, std::size_t idx) const
      {
        const std::size_t size = sizeof(uint16_t);
        const std::size_t offset = base + idx * size;
        const auto* data = PeekObject<le_uint16_t>(offset);
        Require(data != nullptr);
        Ranges.AddService(offset, size);
        return *data;
      }

      struct DataCursors : public std::array<std::size_t, 6>
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
          std::size_t Offset;
          uint_t Counter;

          ChannelState()
            : Offset()
            , Counter()
          {}

          void Skip(uint_t toSkip)
          {
            Counter -= toSkip;
          }

          static bool CompareByCounter(const ChannelState& lh, const ChannelState& rh)
          {
            return lh.Counter < rh.Counter;
          }
        };

        std::array<ChannelState, 6> Channels;

        explicit ParserState(const DataCursors& src)
          : Channels()
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
        const auto minOffset = sizeof(Source);
        const RawPattern& pat = GetPattern(patIndex);
        const DataCursors rangesStarts(pat);
        Require(
            rangesStarts.end()
            == std::find_if(rangesStarts.begin(), rangesStarts.end(), Math::NotInRange(minOffset, Data.Size() - 1)));

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
        for (uint_t chan = 0; chan < 6; ++chan)
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
          else if (0x51 == PeekByte(state.Offset))
          {
            return false;
          }
        }
        return true;
      }

      void ParseLine(ParserState& src, PatternBuilder& patBuilder, Builder& builder) const
      {
        for (uint_t chan = 0; chan < 6; ++chan)
        {
          ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter)
          {
            --state.Counter;
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(state, patBuilder, builder);
        }
      }

      void ParseChannel(ParserState::ChannelState& state, PatternBuilder& patBuilder, Builder& builder) const
      {
        while (state.Offset < Data.Size())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd >= 0xd2)
          {
            state.Counter = cmd - 0xd2;
            break;
          }
          else if (cmd >= 0x72)
          {
            builder.SetNote(cmd - 0x72);
          }
          else if (cmd >= 0x52)
          {
            builder.SetSample(cmd - 0x52);
          }
          else if (cmd >= 0x51)
          {
            break;
          }
          else if (cmd >= 0x50)
          {
            builder.SetRest();
          }
          else if (cmd >= 0x30)
          {
            builder.SetOrnament(cmd - 0x30);
          }
          else if (cmd >= 0x2e)
          {
            builder.SetSwapSampleChannels(cmd > 0x2e);
          }
          else if (cmd >= 0x21)
          {
            builder.SetEnvelope(cmd - 0x21);
          }
          else if (cmd >= 0x11)
          {
            builder.SetAttenuation(cmd - 0x11);
          }
          else if (cmd >= 0x0f)
          {
            builder.SetNoise(cmd != 0x0f ? 3 : cmd - 0x0f);
          }
          else
          {
            patBuilder.SetTempo(cmd + 1);
          }
        }
      }

      void ReadSampleDecodeTable()
      {
        const std::size_t start = Source.SamplesVolumeDecodeTable;
        std::size_t cursor = start;
        SampleDecodeTable.Marker = PeekByte(cursor++);
        SampleDecodeTable.Lenghts.clear();
        while (cursor < Data.Size())
        {
          DecodeTable::CodeAndLen val;
          if ((val.Code = PeekByte(cursor++)))
          {
            val.Len = PeekByte(cursor++);
            SampleDecodeTable.Lenghts.push_back(val);
          }
          else
          {
            break;
          }
        }
        Ranges.Add(start, cursor - start);
      }

      Sample ParseSample(std::size_t start) const
      {
        Sample dst;
        dst.Loop = ~uint_t(0);
        std::size_t cursor = start;
        Sample::Line line;
        for (uint_t devSkip = 0, volSkip = 0;;)
        {
          if (0 == devSkip)
          {
            devSkip = 1;
            bool finished = false;
            for (;;)
            {
              const uint_t val = PeekByte(cursor++);
              if (0 == (val & 1))
              {
                if (val == 0xfe)
                {
                  dst.Loop = dst.Lines.size();
                  continue;
                }
                else if (val == 0xfc)
                {
                  finished = true;
                  break;
                }
                devSkip = 2 + (val >> 1);
              }
              else
              {
                line.ToneEnabled = 0 != (val & 16);
                line.NoiseEnabled = 0 != (val & 32);
                line.NoiseFreq = val >> 6;
                line.ToneDeviation = (((val & 0xe) >> 1) << 8) | PeekByte(cursor++);
                break;
              }
            }
            if (finished)
            {
              break;
            }
          }
          if (0 == volSkip)
          {
            const uint_t val = PeekByte(cursor++);
            if (val == SampleDecodeTable.Marker)
            {
              volSkip = PeekByte(cursor++);
              DecodeLevel(PeekByte(cursor++), line);
            }
            else if ((volSkip = SampleDecodeTable.DecodeLen(val)))
            {
              DecodeLevel(PeekByte(cursor++), line);
            }
            else
            {
              DecodeLevel(val, line);
              volSkip = 1;
            }
          }
          if (const uint_t toSkip = std::min(devSkip, volSkip))
          {
            dst.Lines.resize(dst.Lines.size() + toSkip, line);
            devSkip -= toSkip;
            volSkip -= toSkip;
          }
        }
        Ranges.Add(start, cursor - start);
        dst.Loop = std::min<uint_t>(dst.Loop, dst.Lines.size());
        return dst;
      }

      static void DecodeLevel(uint_t val, Sample::Line& line)
      {
        line.LeftLevel = val & 15;
        line.RightLevel = val >> 4;
      }

      /*
              ld	a,(ix++0Dh) ;ornament offset
                      dec	(ix++14h)   ;ornament counter
                      jr	nz,loc_0_82B1
                      ld	c,1
                      ld	l,(ix++6)   ;ornament ptr
                      ld	h,(ix++7)
      loc_0_829F:
                      ld	a,(hl)
                      inc	hl
                      cp	60h ; '`'
                      jr	nc,loc_0_824F
                      ld	(ix++14h),c
                      ld	(ix++0Dh),a
                      ld	(ix++6),l
                      ld	(ix++7),h
      loc_0_82B1:
                      add	a,(ix++0Eh)

          <...>

      loc_0_824F:
                      inc	a
                      jr	z,loc_0_825A
                      inc	a
                      jr	z,loc_0_8262
                      sub	60h ; '`'
                      ld	c,a
                      jr	loc_0_829F
      loc_0_825A:
                      ld	l,(ix++8)
                      ld	h,(ix++9)
                      jr	loc_0_829F
      loc_0_8262:
                      ld	(ix++8),l
                      ld	(ix++9),h
                      jr	loc_0_829F
      */

      Ornament ParseOrnament(std::size_t start) const
      {
        Ornament dst;
        dst.Loop = ~uint_t(0);
        std::size_t cursor = start;
        uint_t line = 0;
        uint_t skip = 1;
        for (;;)
        {
          const uint_t val = PeekByte(cursor++);
          if (val < 0x60)
          {
            line = val;
          }
          else if (val == 0xff)
          {
            break;
          }
          else if (val == 0xfe)
          {
            dst.Loop = dst.Lines.size();
            continue;
          }
          else
          {
            skip = val + 2 - 0x60;
            continue;
          }
          dst.Lines.resize(dst.Lines.size() + skip, line);
          skip = 1;
        }
        Ranges.Add(start, cursor - start);
        dst.Loop = std::min<uint_t>(dst.Loop, dst.Lines.size());
        return dst;
      }

    private:
      const Binary::View Data;
      RangesMap Ranges;
      const RawHeader& Source;
      DecodeTable SampleDecodeTable;
    };

    enum AreaTypes
    {
      HEADER,
      SAMPLES_TABLE,
      ORNAMENTS_TABLE,
      SAMPLESDECODE_TABLE,
      POSITIONS_TABLE,
      PATTERNS_TABLE,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        const std::size_t samples = header.SamplesOffset;
        const std::size_t ornaments = header.OrnamentsOffset;
        const std::size_t samplesDecode = header.SamplesVolumeDecodeTable;
        AddArea(SAMPLES_TABLE, samples);
        AddArea(SAMPLESDECODE_TABLE, samplesDecode);
        if (samples != ornaments && samplesDecode != ornaments)
        {
          AddArea(ORNAMENTS_TABLE, ornaments);
        }
        AddArea(POSITIONS_TABLE, header.PositionsOffset);
        AddArea(PATTERNS_TABLE, header.PatternsOffset);
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return GetAreaSize(HEADER) >= sizeof(RawHeader) && Undefined == GetAreaSize(END);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES_TABLE);
        // at least one record
        return size != Undefined && size >= sizeof(uint16_t);
      }

      bool CheckOrnaments() const
      {
        // at least one record or nothing
        if (Undefined == GetAreaAddress(ORNAMENTS_TABLE))
        {
          return true;
        }
        const std::size_t size = GetAreaSize(ORNAMENTS_TABLE);
        return size != Undefined && size >= sizeof(uint16_t);
      }

      bool CheckPositions() const
      {
        const std::size_t size = GetAreaSize(POSITIONS_TABLE);
        // single entry+limiter
        return size != Undefined && size >= 2;
      }

      bool CheckPatterns() const
      {
        const std::size_t size = GetAreaSize(PATTERNS_TABLE);
        return size != Undefined && size >= sizeof(RawPattern);
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
      if (!areas.CheckSamples())
      {
        return false;
      }
      if (!areas.CheckOrnaments())
      {
        return false;
      }
      if (!areas.CheckPositions())
      {
        return false;
      }
      if (!areas.CheckPatterns())
      {
        return false;
      }
      return true;
    }

    const auto FORMAT =
        "(?00-7f)"
        "(?00-7f)"
        "(?00-7f)"
        "(?00-7f)"
        "(?00-7f)"
        "'E'T'r'a'c'k'e'r' '('C')' 'B'Y' 'E'S'I'."
        ""_sv;

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
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);
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
  }  // namespace ETracker

  Decoder::Ptr CreateETrackerDecoder()
  {
    return MakePtr<ETracker::Decoder>();
  }
}  // namespace Formats::Chiptune
