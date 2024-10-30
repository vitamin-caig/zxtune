/**
 *
 * @file
 *
 * @brief  ASCSoundMaster support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/ascsoundmaster.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/metainfo.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/casing.h>
#include <strings/optimize.h>
#include <strings/trim.h>
#include <tools/indices.h>
#include <tools/range_checker.h>
// std includes
#include <array>
#include <cstring>

namespace Formats::Chiptune
{
  namespace ASCSoundMaster
  {
    const Debug::Stream Dbg("Formats::Chiptune::ASCSoundMaster");

    const std::size_t SAMPLES_COUNT = 32;
    const std::size_t MAX_SAMPLE_SIZE = 150;
    const std::size_t ORNAMENTS_COUNT = 32;
    const std::size_t MAX_ORNAMENT_SIZE = 30;
    // according to manual
    const std::size_t MIN_PATTERN_SIZE = 1;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 32;  // TODO

    /*

      Typical module structure

      Header
      Optional id
      Patterns list
      Patterns data
      Samples list
      Samples data
      Ornaments list
      Ornaments data
    */

    struct RawHeaderVer0
    {
      uint8_t Tempo;
      le_uint16_t PatternsOffset;
      le_uint16_t SamplesOffset;
      le_uint16_t OrnamentsOffset;
      uint8_t Length;
      uint8_t Positions[1];

      // for same static interface
      static const std::size_t Loop = 0;
    };

    struct RawHeaderVer1
    {
      uint8_t Tempo;
      uint8_t Loop;
      le_uint16_t PatternsOffset;
      le_uint16_t SamplesOffset;
      le_uint16_t OrnamentsOffset;
      uint8_t Length;
      uint8_t Positions[1];
    };

    const uint8_t ASC_ID_1[] = {'A', 'S', 'M', ' ', 'C', 'O', 'M', 'P', 'I', 'L',
                                'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '};

    struct RawId
    {
      uint8_t Identifier1[19];  //'ASM COMPILATION OF '
      std::array<char, 20> Title;
      std::array<char, 4> Identifier2;  //' BY ' or smth similar
      std::array<char, 20> Author;

      bool Check() const
      {
        static_assert(sizeof(ASC_ID_1) == sizeof(Identifier1), "Invalid layout");
        return 0 == std::memcmp(Identifier1, ASC_ID_1, sizeof(Identifier1));
      }

      bool HasAuthor() const
      {
        const auto BY_DELIMITER = "BY"sv;
        const auto trimId = Strings::TrimSpaces(MakeStringView(Identifier2));
        return Strings::EqualNoCaseAscii(trimId, BY_DELIMITER);
      }
    };

    struct RawPattern
    {
      std::array<le_uint16_t, 3> Offsets;  // from start of patterns
    };

    struct RawOrnamentsList
    {
      std::array<le_uint16_t, ORNAMENTS_COUNT> Offsets;
    };

    struct RawOrnament
    {
      struct Line
      {
        // BEFooooo
        // OOOOOOOO

        // o - noise offset (signed)
        // B - loop begin
        // E - loop end
        // F - finished
        // O - note offset (signed)
        uint8_t LoopAndNoiseOffset;
        int8_t NoteOffset;

        bool IsLoopBegin() const
        {
          return 0 != (LoopAndNoiseOffset & 128);
        }

        bool IsLoopEnd() const
        {
          return 0 != (LoopAndNoiseOffset & 64);
        }

        bool IsFinished() const
        {
          return 0 != (LoopAndNoiseOffset & 32);
        }

        int_t GetNoiseOffset() const
        {
          return static_cast<int8_t>(LoopAndNoiseOffset * 8) / 8;
        }
      };
      Line Data[1];
    };

    struct RawSamplesList
    {
      std::array<le_uint16_t, SAMPLES_COUNT> Offsets;
    };

    struct RawSample
    {
      struct Line
      {
        // BEFaaaaa
        // TTTTTTTT
        // LLLLnCCt
        // a - adding
        // B - loop begin
        // E - loop end
        // F - finished
        // T - tone deviation
        // L - level
        // n - noise mask
        // C - command
        // t - tone mask

        uint8_t LoopAndAdding;
        int8_t ToneDeviation;
        uint8_t LevelAndMasks;

        bool IsLoopBegin() const
        {
          return 0 != (LoopAndAdding & 128);
        }

        bool IsLoopEnd() const
        {
          return 0 != (LoopAndAdding & 64);
        }

        bool IsFinished() const
        {
          return 0 != (LoopAndAdding & 32);
        }

        int_t GetAdding() const
        {
          return static_cast<int8_t>(LoopAndAdding * 8) / 8;
        }

        uint_t GetLevel() const
        {
          return LevelAndMasks >> 4;
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelAndMasks & 8);
        }

        uint_t GetCommand() const
        {
          return (LevelAndMasks & 6) >> 1;
        }

        bool GetToneMask() const
        {
          return 0 != (LevelAndMasks & 1);
        }
      };

      enum
      {
        EMPTY,
        ENVELOPE,
        DECVOLADD,
        INCVOLADD
      };
      Line Data[1];
    };

    static_assert(sizeof(RawHeaderVer0) * alignof(RawHeaderVer0) == 9, "Invalid layout");
    static_assert(sizeof(RawHeaderVer1) * alignof(RawHeaderVer1) == 10, "Invalid layout");
    static_assert(sizeof(RawId) * alignof(RawId) == 63, "Invalid layout");
    static_assert(sizeof(RawPattern) * alignof(RawPattern) == 6, "Invalid layout");
    static_assert(sizeof(RawOrnamentsList) * alignof(RawOrnamentsList) == 64, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 2, "Invalid layout");
    static_assert(sizeof(RawSamplesList) * alignof(RawSamplesList) == 64, "Invalid layout");
    static_assert(sizeof(RawSample) * alignof(RawSample) == 3, "Invalid layout");

    template<class RawHeader>
    std::size_t GetHeaderSize(const RawHeader& hdr)
    {
      return offsetof(RawHeader, Positions) + hdr.Length;
    }

    struct HeaderTraits
    {
      const std::size_t Size;
      const std::size_t PatternsOffset;
      const std::size_t SamplesOffset;
      const std::size_t OrnamentsOffset;
      const uint_t Tempo;
      const uint_t Loop;
      const uint8_t* const Positions;
      const uint_t Length;

      template<class RawHeader>
      explicit HeaderTraits(const RawHeader& hdr)
        : Size(GetHeaderSize(hdr))
        , PatternsOffset(hdr.PatternsOffset)
        , SamplesOffset(hdr.SamplesOffset)
        , OrnamentsOffset(hdr.OrnamentsOffset)
        , Tempo(hdr.Tempo)
        , Loop(hdr.Loop)
        , Positions(hdr.Positions)
        , Length(hdr.Length)
      {}

      bool Check() const
      {
        return Math::InRange<uint_t>(Tempo, 0x03, 0x32) && Math::InRange<uint_t>(Loop, 0x00, 0x63)
               && Math::InRange<uint_t>(Length, 0x01, 0x64)
               && Positions + Length == std::find_if(Positions, Positions + Length, [](uint_t pos) {
                    return pos >= MAX_PATTERNS_COUNT;
                  });
      }

      template<class RawHeader>
      static HeaderTraits Create(Binary::View data)
      {
        const auto* const hdr = data.As<RawHeader>();
        Require(hdr != nullptr);
        return HeaderTraits(*hdr);
      }
    };

    using CreateHeaderFunc = HeaderTraits (*)(Binary::View);

    struct VersionTraits
    {
      const std::size_t MinSize;
      const std::size_t MaxSize;
      const StringView Description;
      const StringView Format;
      const CreateHeaderFunc CreateHeader;

      Binary::View CreateContainer(Binary::View rawData) const
      {
        return rawData.Size() >= MinSize ? rawData.SubView(0, MaxSize) : Binary::View(nullptr, 0);
      }
    };

    struct Version0
    {
      static const VersionTraits TRAITS;
      using RawHeader = RawHeaderVer0;
    };

    const VersionTraits Version0::TRAITS = {255, 0x2400,  //~9k
                                            "ASC Sound Master v0.x"sv,
                                            "03-32"     // tempo
                                            "09-ab 00"  // patterns
                                            "? 00-21"   // samples
                                            "? 00-22"   // ornaments
                                            "01-64"     // length
                                            "00-1f"     // first position
                                            ""sv,
                                            &HeaderTraits::Create<RawHeaderVer0>};

    struct Version1
    {
      static const VersionTraits TRAITS;
      using RawHeader = RawHeaderVer1;
    };

    const VersionTraits Version1::TRAITS = {256, 0x3a00,  //~15k
                                            "ASC Sound Master v1.x"sv,
                                            "03-32"     // tempo
                                            "00-63"     // loop
                                            "0a-ac 00"  // patterns
                                            "? 00-35"   // samples
                                            "? 00-37"   // ornaments
                                            "01-64"     // length
                                            "00-1f"     // first position
                                            ""sv,
                                            &HeaderTraits::Create<RawHeaderVer1>};

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
      void SetVolume(uint_t /*vol*/) override {}
      void SetEnvelopeType(uint_t /*type*/) override {}
      void SetEnvelopeTone(uint_t /*tone*/) override {}
      void SetEnvelope() override {}
      void SetNoEnvelope() override {}
      void SetNoise(uint_t /*val*/) override {}
      void SetContinueSample() override {}
      void SetContinueOrnament() override {}
      void SetGlissade(int_t /*val*/) override {}
      void SetSlide(int_t /*steps*/, bool /*useToneSliding*/) override {}
      void SetVolumeSlide(uint_t /*period*/, int_t /*delta*/) override {}
      void SetBreakSample() override {}
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

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
        assert(index == 0 || UsedOrnaments.Contain(index));
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
        Delegate.StartChannel(index);
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

      void SetEnvelopeType(uint_t type) override
      {
        return Delegate.SetEnvelopeType(type);
      }

      void SetEnvelopeTone(uint_t tone) override
      {
        return Delegate.SetEnvelopeTone(tone);
      }

      void SetEnvelope() override
      {
        return Delegate.SetEnvelope();
      }

      void SetNoEnvelope() override
      {
        return Delegate.SetNoEnvelope();
      }

      void SetNoise(uint_t val) override
      {
        return Delegate.SetNoise(val);
      }

      void SetContinueSample() override
      {
        return Delegate.SetContinueSample();
      }

      void SetContinueOrnament() override
      {
        return Delegate.SetContinueOrnament();
      }

      void SetGlissade(int_t val) override
      {
        return Delegate.SetGlissade(val);
      }

      void SetSlide(int_t steps, bool useToneSliding) override
      {
        return Delegate.SetSlide(steps, useToneSliding);
      }

      void SetVolumeSlide(uint_t period, int_t delta) override
      {
        return Delegate.SetVolumeSlide(period, delta);
      }

      void SetBreakSample() override
      {
        return Delegate.SetBreakSample();
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
      Format(Binary::View data, const HeaderTraits& hdr)
        : Data(data)
        , Ranges(data.Size())
        , Header(hdr)
        , Id(GetObject<RawId>(Header.Size))
      {
        Ranges.AddService(0, Header.Size);
        if (Id.Check())
        {
          Ranges.AddService(Header.Size, sizeof(Id));
        }
      }

      void ParseCommonProperties(const VersionTraits& version, Builder& builder) const
      {
        builder.SetInitialTempo(Header.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(version.Description);
        if (Id.Check())
        {
          if (Id.HasAuthor())
          {
            meta.SetTitle(Strings::OptimizeAscii(Id.Title));
            meta.SetAuthor(Strings::OptimizeAscii(Id.Author));
          }
          else
          {
            meta.SetTitle(Strings::OptimizeAscii(MakeStringView(Id.Title.data(), &Id.Author.back() + 1)));
          }
        }
      }

      void ParsePositions(Builder& builder) const
      {
        Positions pos;
        pos.Loop = Header.Loop;
        pos.Lines.assign(Header.Positions, Header.Positions + Header.Length);
        Dbg("Positions: {} entries, loop to {}", pos.Lines.size(), pos.Loop);
        builder.SetPositions(std::move(pos));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: {} to parse", pats.Count());
        const std::size_t baseOffset = Header.PatternsOffset;
        bool hasValidPatterns = false;
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern {}", patIndex);
          if (ParsePattern(baseOffset, patIndex, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Dbg("Samples: {} to parse", samples.Count());
        const std::size_t baseOffset = Header.SamplesOffset;
        const auto& list = GetServiceObject<RawSamplesList>(baseOffset);
        std::size_t prevOffset = list.Offsets[0] - sizeof(RawSample::Line);
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Dbg("Parse sample {}", samIdx);
          const std::size_t curOffset = list.Offsets[samIdx];
          Require(curOffset > prevOffset);
          Require(0 == (curOffset - prevOffset) % sizeof(RawSample::Line));
          builder.SetSample(samIdx, ParseSample(baseOffset + curOffset));
          prevOffset = curOffset;
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: {} to parse", ornaments.Count());
        const std::size_t baseOffset = Header.OrnamentsOffset;
        const auto& list = GetServiceObject<RawOrnamentsList>(baseOffset);
        std::size_t prevOffset = list.Offsets[0] - sizeof(RawOrnament::Line);
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Dbg("Parse ornament {}", ornIdx);
          const std::size_t curOffset = list.Offsets[ornIdx];
          Require(curOffset > prevOffset);
          Require(0 == (curOffset - prevOffset) % sizeof(RawOrnament::Line));
          builder.SetOrnament(ornIdx, ParseOrnament(baseOffset + curOffset));
          prevOffset = curOffset;
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
      const T& GetObject(std::size_t offset) const
      {
        const T* src = PeekObject<T>(offset);
        Require(src != nullptr);
        Ranges.Add(offset, sizeof(T));
        return *src;
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

      struct DataCursors : public std::array<std::size_t, 3>
      {
        DataCursors(const RawPattern& src, std::size_t baseOffset)
        {
          std::transform(src.Offsets.begin(), src.Offsets.end(), begin(),
                         [baseOffset](auto o) { return baseOffset + o; });
        }
      };

      struct ParserState
      {
        struct ChannelState
        {
          std::size_t Offset = 0;
          uint_t Period = 0;
          uint_t Counter = 0;
          bool Envelope = false;

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

      bool ParsePattern(std::size_t baseOffset, uint_t patIndex, Builder& builder) const
      {
        const auto& pat = GetServiceObject<RawPattern>(baseOffset + patIndex * sizeof(RawPattern));
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        const DataCursors rangesStarts(pat, baseOffset);
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
          if (state.Offset >= Data.Size() || (0 == chan && 0xff == PeekByte(state.Offset)))
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
          if (cmd <= 0x55)  // note
          {
            builder.SetNote(cmd);
            if (state.Envelope)
            {
              builder.SetEnvelopeTone(PeekByte(state.Offset++));
            }
            break;
          }
          else if (cmd <= 0x5d)  // stop
          {
            break;
          }
          else if (cmd == 0x5e)  // break sample
          {
            builder.SetBreakSample();
            break;
          }
          else if (cmd == 0x5f)  // rest
          {
            builder.SetRest();
            break;
          }
          else if (cmd <= 0x9f)  // skip
          {
            state.Period = cmd - 0x60;
          }
          else if (cmd <= 0xbf)  // sample
          {
            builder.SetSample(cmd - 0xa0);
          }
          else if (cmd <= 0xdf)  // ornament
          {
            builder.SetOrnament(cmd - 0xc0);
          }
          else if (cmd == 0xe0)  // envelope full vol
          {
            builder.SetVolume(15);
            builder.SetEnvelope();
            state.Envelope = true;
          }
          else if (cmd <= 0xef)  // noenvelope vol
          {
            builder.SetVolume(cmd - 0xe0);
            builder.SetNoEnvelope();
            state.Envelope = false;
          }
          else if (cmd == 0xf0)  // noise
          {
            builder.SetNoise(PeekByte(state.Offset++));
          }
          else if ((cmd & 0xfc) == 0xf0)  // 0xf1, 0xf2, 0xf3 - continue sample or ornament
          {
            if (cmd & 1)
            {
              builder.SetContinueSample();
            }
            if (cmd & 2)
            {
              builder.SetContinueOrnament();
            }
          }
          else if (cmd == 0xf4)  // tempo
          {
            const uint_t newTempo = PeekByte(state.Offset++);
            // do not check tempo
            patBuilder.SetTempo(newTempo);
          }
          else if (cmd <= 0xf6)  // slide
          {
            const int_t slide = ((cmd == 0xf5) ? -16 : 16) * PeekByte(state.Offset++);
            builder.SetGlissade(slide);
          }
          else if (cmd == 0xf7 || cmd == 0xf9)  // stepped slide
          {
            if (cmd == 0xf7)
            {
              builder.SetContinueSample();
            }
            builder.SetSlide(static_cast<int8_t>(PeekByte(state.Offset++)), cmd == 0xf7);
          }
          else if ((cmd & 0xf9) == 0xf8)  // 0xf8, 0xfa, 0xfc, 0xfe - envelope
          {
            builder.SetEnvelopeType(cmd & 0xf);
          }
          else if (cmd == 0xfb)  // amp delay
          {
            const uint_t step = PeekByte(state.Offset++);
            builder.SetVolumeSlide(step & 31, step & 32 ? -1 : 1);
          }
        }
      }

      Sample ParseSample(std::size_t offset) const
      {
        const auto& src = GetObject<RawSample>(offset);
        Sample result;
        const std::size_t availSize = (Data.Size() - offset) / sizeof(RawSample::Line);
        for (std::size_t idx = 0, lim = std::min(availSize, MAX_SAMPLE_SIZE); idx != lim; ++idx)
        {
          const RawSample::Line& srcLine = src.Data[idx];
          const Sample::Line& dstLine = ParseSampleLine(srcLine);
          result.Lines.push_back(dstLine);
          if (srcLine.IsLoopBegin())
          {
            result.Loop = idx;
          }
          if (srcLine.IsLoopEnd())
          {
            result.LoopLimit = idx;
          }
          if (srcLine.IsFinished())
          {
            break;
          }
        }
        Ranges.Add(offset, result.Lines.size() * sizeof(RawSample::Line));
        return result;
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& src)
      {
        Sample::Line result;
        result.Level = src.GetLevel();
        result.ToneDeviation = src.ToneDeviation;
        result.ToneMask = src.GetToneMask();
        result.NoiseMask = src.GetNoiseMask();
        result.Adding = src.GetAdding();
        switch (src.GetCommand())
        {
        case RawSample::ENVELOPE:
          result.EnableEnvelope = true;
          break;
        case RawSample::DECVOLADD:
          result.VolSlide = -1;
          break;
        case RawSample::INCVOLADD:
          result.VolSlide = 1;
          break;
        default:
          break;
        }
        return result;
      }

      Ornament ParseOrnament(std::size_t offset) const
      {
        Ornament result;
        if (const auto* src = PeekObject<RawOrnament>(offset))
        {
          const std::size_t availSize = (Data.Size() - offset) / sizeof(RawOrnament::Line);
          for (std::size_t idx = 0, lim = std::min(availSize, MAX_ORNAMENT_SIZE); idx != lim; ++idx)
          {
            const RawOrnament::Line& srcLine = src->Data[idx];
            const Ornament::Line& dstLine = ParseOrnamentLine(srcLine);
            result.Lines.push_back(dstLine);
            if (srcLine.IsLoopBegin())
            {
              result.Loop = idx;
            }
            if (srcLine.IsLoopEnd())
            {
              result.LoopLimit = idx;
            }
            if (srcLine.IsFinished())
            {
              break;
            }
          }
          Ranges.Add(offset, result.Lines.size() * sizeof(RawOrnament::Line));
        }
        else
        {
          Dbg("Stub ornament");
        }
        return result;
      }

      static Ornament::Line ParseOrnamentLine(const RawOrnament::Line& src)
      {
        Ornament::Line result;
        result.NoteAddon = src.NoteOffset;
        result.NoiseAddon = src.GetNoiseOffset();
        return result;
      }

    private:
      Binary::View Data;
      RangesMap Ranges;
      const HeaderTraits& Header;
      const RawId& Id;
    };

    enum AreaTypes
    {
      HEADER,
      IDENTIFIER,
      PATTERNS,
      SAMPLES,
      ORNAMENTS,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const VersionTraits& traits, Binary::View data)
        : Header(traits.CreateHeader(data))
      {
        AddArea(HEADER, 0);
        if (Header.Size + sizeof(RawId) <= data.Size())
        {
          const auto* id = data.SubView(Header.Size).As<RawId>();
          if (id->Check())
          {
            AddArea(IDENTIFIER, Header.Size);
          }
        }
        AddArea(PATTERNS, Header.PatternsOffset);
        AddArea(SAMPLES, Header.SamplesOffset);
        AddArea(ORNAMENTS, Header.OrnamentsOffset);
        AddArea(END, data.Size());
      }

      bool CheckHeader() const
      {
        if (!Header.Check())
        {
          return false;
        }
        if (Header.Size > GetAreaSize(HEADER) || Undefined != GetAreaSize(END))
        {
          return false;
        }
        const std::size_t idSize = GetAreaSize(IDENTIFIER);
        return idSize == 0 || idSize >= sizeof(RawId);
      }

      bool CheckSamples() const
      {
        if (GetAreaAddress(SAMPLES) < GetAreaAddress(PATTERNS))
        {
          return false;
        }
        const std::size_t size = GetAreaSize(SAMPLES);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawSamplesList);
        return requiredSize <= size;
      }

      bool CheckOrnaments() const
      {
        if (GetAreaAddress(ORNAMENTS) < GetAreaAddress(SAMPLES))
        {
          return false;
        }
        const std::size_t size = GetAreaSize(ORNAMENTS);
        if (Undefined == size)
        {
          return false;
        }
        const std::size_t requiredSize = sizeof(RawOrnamentsList);
        return requiredSize <= size;
      }

    private:
      const HeaderTraits Header;
    };

    bool Check(const Areas& areas, Binary::View data)
    {
      if (!areas.CheckHeader())
      {
        return false;
      }
      if (!areas.CheckSamples())
      {
        return false;
      }
      if (const auto* samplesList = data.SubView(areas.GetAreaAddress(SAMPLES)).As<RawSamplesList>())
      {
        if (samplesList->Offsets[0] != sizeof(*samplesList))
        {
          return false;
        }
      }
      else
      {
        return false;
      }
      if (!areas.CheckOrnaments())
      {
        return false;
      }
      if (const auto* ornamentsList = data.SubView(areas.GetAreaAddress(ORNAMENTS)).As<RawOrnamentsList>())
      {
        if (ornamentsList->Offsets[0] < sizeof(*ornamentsList))
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

    bool Check(const VersionTraits& version, Binary::View data)
    {
      const Areas areas(version, data);
      return Check(areas, data);
    }

    Formats::Chiptune::Container::Ptr Parse(const VersionTraits& version, const Binary::Container& rawData,
                                            Builder& target)
    {
      const auto data = version.CreateContainer(rawData);
      if (!data || !Check(version, data))
      {
        return {};
      }
      try
      {
        const HeaderTraits& header = version.CreateHeader(data);
        const Format format(data, header);
        format.ParseCommonProperties(version, target);

        StatisticCollectingBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

        Require(format.GetSize() >= version.MinSize);
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

    template<class Version>
    Binary::Container::Ptr InsertMetaInformation(const Binary::Container& rawData, Binary::View info)
    {
      const VersionTraits& version = Version::TRAITS;
      if (const auto parsed = Parse(version, rawData, GetStubBuilder()))
      {
        const auto& data = version.CreateContainer(*parsed);
        const auto& header = *data.As<typename Version::RawHeader>();
        const std::size_t headerSize = GetHeaderSize(header);
        const std::size_t infoSize = info.Size();
        const auto patch = PatchedDataBuilder::Create(*parsed);
        const auto* id = data.SubView(headerSize).As<RawId>();
        if (id && id->Check())
        {
          patch->OverwriteData(headerSize, info);
        }
        else
        {
          patch->InsertData(headerSize, info);
          patch->FixLEWord(offsetof(typename Version::RawHeader, PatternsOffset), infoSize);
          patch->FixLEWord(offsetof(typename Version::RawHeader, SamplesOffset), infoSize);
          patch->FixLEWord(offsetof(typename Version::RawHeader, OrnamentsOffset), infoSize);
        }
        return patch->GetResult();
      }
      else
      {
        return {};
      }
    }

    class VersionedDecoder : public Decoder
    {
    public:
      explicit VersionedDecoder(const VersionTraits& version)
        : Version(version)
        , Header(Binary::CreateFormat(version.Format, version.MinSize))
      {}

      StringView GetDescription() const override
      {
        return Version.Description;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Header;
      }

      bool Check(Binary::View rawData) const override
      {
        return Header->Match(rawData) && ASCSoundMaster::Check(Version, rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        Builder& stub = GetStubBuilder();
        return ASCSoundMaster::Parse(Version, rawData, stub);
      }

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const override
      {
        return ASCSoundMaster::Parse(Version, data, target);
      }

    private:
      const VersionTraits& Version;
      const Binary::Format::Ptr Header;
    };

    namespace Ver0
    {
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
      {
        return Parse(Version0::TRAITS, data, target);
      }

      Binary::Container::Ptr InsertMetaInformation(const Binary::Container& data, Binary::View info)
      {
        return ASCSoundMaster::InsertMetaInformation<Version0>(data, info);
      }

      Decoder::Ptr CreateDecoder()
      {
        return MakePtr<VersionedDecoder>(Version0::TRAITS);
      }
    }  // namespace Ver0

    namespace Ver1
    {
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
      {
        return Parse(Version1::TRAITS, data, target);
      }

      Binary::Container::Ptr InsertMetaInformation(const Binary::Container& data, Binary::View info)
      {
        return ASCSoundMaster::InsertMetaInformation<Version1>(data, info);
      }

      Decoder::Ptr CreateDecoder()
      {
        return MakePtr<VersionedDecoder>(Version1::TRAITS);
      }
    }  // namespace Ver1
  }    // namespace ASCSoundMaster

  Decoder::Ptr CreateASCSoundMaster0xDecoder()
  {
    return ASCSoundMaster::Ver0::CreateDecoder();
  }

  Decoder::Ptr CreateASCSoundMaster1xDecoder()
  {
    return ASCSoundMaster::Ver1::CreateDecoder();
  }
}  // namespace Formats::Chiptune
