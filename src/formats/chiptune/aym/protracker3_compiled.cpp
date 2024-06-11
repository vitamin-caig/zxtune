/**
 *
 * @file
 *
 * @brief  ProTracker v3.x compiled modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/protracker3_detail.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
#include <range_checker.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/casing.h>
#include <strings/sanitize.h>
#include <strings/trim.h>
// std includes
#include <array>
#include <cctype>

namespace Formats::Chiptune
{
  namespace ProTracker3
  {
    const Debug::Stream Dbg("Formats::Chiptune::ProTracker3");

    const std::size_t MIN_SIZE = 200;
    const std::size_t MAX_SIZE = 0xc000;

    struct RawId
    {
      std::array<char, 32> TrackName;
      std::array<char, 4> Optional2;  //' by '
      std::array<char, 32> TrackAuthor;

      bool HasAuthor() const
      {
        const auto BY_DELIMITER = "BY"_sv;
        const auto trimId = Strings::TrimSpaces(Optional2);
        return Strings::EqualNoCaseAscii(trimId, BY_DELIMITER);
      }
    };

    struct RawHeader
    {
      std::array<char, 13> Id;  //'ProTracker 3.'
      uint8_t Subversion;
      std::array<char, 16> Optional1;  //' compilation of '
      RawId Metainfo;
      uint8_t Mode;
      uint8_t FreqTableNum;
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      le_uint16_t PatternsOffset;
      std::array<le_uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
      std::array<le_uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
      uint8_t Positions[1];  // finished by marker

      StringView GetProgram() const
      {
        const auto COMPILATION_OF = "COMPILATION OF"_sv;
        const auto opt = Strings::TrimSpaces(Optional1);
        return Strings::EqualNoCaseAscii(opt, COMPILATION_OF) ? StringView(Id.data(), Optional1.data())
                                                              : StringView(Id.data(), &Optional1.back() + 1);
      }

      uint_t GetVersion() const
      {
        return std::isdigit(Subversion) ? Subversion - '0' : 6;
      }
    };

    const uint8_t POS_END_MARKER = 0xff;

    struct RawObject
    {
      uint8_t Loop;
      uint8_t Size;

      uint_t GetSize() const
      {
        return Size;
      }
    };

    struct RawSample : RawObject
    {
      struct Line
      {
        // SUoooooE
        // NkKTLLLL
        // OOOOOOOO
        // OOOOOOOO
        // S - vol slide
        // U - vol slide up
        // o - noise or env offset
        // E - env mask
        // L - level
        // T - tone mask
        // K - keep noise or env offset
        // k - keep tone offset
        // N - noise mask
        // O - tone offset
        uint8_t VolSlideEnv;
        uint8_t LevelKeepers;
        le_int16_t ToneOffset;

        bool GetEnvelopeMask() const
        {
          return 0 != (VolSlideEnv & 1);
        }

        int_t GetNoiseOrEnvelopeOffset() const
        {
          const uint8_t noeoff = (VolSlideEnv & 62) >> 1;
          return static_cast<int8_t>(noeoff & 16 ? noeoff | 0xf0 : noeoff);
        }

        int_t GetVolSlide() const
        {
          return (VolSlideEnv & 128) ? ((VolSlideEnv & 64) ? +1 : -1) : 0;
        }

        uint_t GetLevel() const
        {
          return LevelKeepers & 15;
        }

        bool GetToneMask() const
        {
          return 0 != (LevelKeepers & 16);
        }

        bool GetKeepNoiseOrEnvelopeOffset() const
        {
          return 0 != (LevelKeepers & 32);
        }

        bool GetKeepToneOffset() const
        {
          return 0 != (LevelKeepers & 64);
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelKeepers & 128);
        }

        int_t GetToneOffset() const
        {
          return ToneOffset;
        }
      };

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }

      Line GetLine(uint_t idx) const
      {
        static_assert(0 == (sizeof(Line) & (sizeof(Line) - 1)), "Invalid layout");
        const uint_t maxLines = 256 / sizeof(Line);
        const Line* const src = safe_ptr_cast<const Line*>(this + 1);
        return src[idx % maxLines];
      }
    };

    struct RawOrnament : RawObject
    {
      using Line = int8_t;

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

    struct RawPattern
    {
      std::array<le_uint16_t, 3> Offsets;

      bool Check() const
      {
        return Offsets[0] && Offsets[1] && Offsets[2];
      }
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 202, "Invalid layout");
    static_assert(sizeof(RawSample) * alignof(RawSample) == 2, "Invalid layout");
    static_assert(sizeof(RawSample::Line) * alignof(RawSample::Line) == 4, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 2, "Invalid layout");

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      void SetVersion(uint_t /*version*/) override {}
      void SetNoteTable(NoteTable /*table*/) override {}
      void SetMode(uint_t /*mode*/) override {}
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
      void SetGlissade(uint_t /*period*/, int_t /*val*/) override {}
      void SetNoteGliss(uint_t /*period*/, int_t /*val*/, uint_t /*limit*/) override {}
      void SetSampleOffset(uint_t /*offset*/) override {}
      void SetOrnamentOffset(uint_t /*offset*/) override {}
      void SetVibrate(uint_t /*ontime*/, uint_t /*offtime*/) override {}
      void SetEnvelopeSlide(uint_t /*period*/, int_t /*val*/) override {}
      void SetEnvelope(uint_t /*type*/, uint_t /*value*/) override {}
      void SetNoEnvelope() override {}
      void SetNoiseBase(uint_t /*val*/) override {}
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

    std::size_t GetHeaderSize(Binary::View data)
    {
      if (const auto* hdr = data.As<RawHeader>())
      {
        const auto* const dataBegin = safe_ptr_cast<const uint8_t*>(hdr->Id.data());
        const auto* const dataEnd =
            dataBegin + std::min(data.Size(), MAX_POSITIONS_COUNT + offsetof(RawHeader, Positions) + 1);
        const auto* const lastPosition = std::find(hdr->Positions, dataEnd, POS_END_MARKER);
        if (lastPosition != dataEnd && std::all_of(hdr->Positions, lastPosition, [](auto b) { return 0 == b % 3; }))
        {
          return lastPosition + 1 - dataBegin;
        }
      }
      return 0;
    }

    void CheckTempo(uint_t tempo)
    {
      Require(tempo >= 1);
    }

    class Format
    {
    public:
      explicit Format(Binary::View data)
        : Data(data)
        , Ranges(Data.Size())
        , Source(*Data.As<RawHeader>())
      {
        Ranges.AddService(0, GetHeaderSize(Data));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Strings::Sanitize(Source.GetProgram()));
        const RawId& id = Source.Metainfo;
        if (id.HasAuthor())
        {
          meta.SetTitle(Strings::Sanitize(id.TrackName));
          meta.SetAuthor(Strings::Sanitize(id.TrackAuthor));
        }
        else
        {
          meta.SetTitle(Strings::Sanitize(StringView(id.TrackName.data(), &id.TrackAuthor.back() + 1)));
        }
        builder.SetVersion(Source.GetVersion());
        if (Math::InRange<uint_t>(Source.FreqTableNum, PROTRACKER, NATURAL))
        {
          builder.SetNoteTable(static_cast<NoteTable>(Source.FreqTableNum));
        }
        else
        {
          builder.SetNoteTable(PROTRACKER);
        }
        builder.SetMode(Source.Mode);
        const uint_t tempo = Source.Tempo;
        CheckTempo(tempo);
        builder.SetInitialTempo(tempo);
      }

      void ParsePositions(Builder& builder) const
      {
        Positions positions;
        for (const uint8_t* pos = Source.Positions; *pos != POS_END_MARKER; ++pos)
        {
          const uint_t patNum = *pos;
          Require(0 == patNum % 3);
          positions.Lines.push_back(patNum / 3);
        }
        Require(Math::InRange<std::size_t>(positions.GetSize(), 1, MAX_POSITIONS_COUNT));
        positions.Loop = Source.Loop;
        Dbg("Positions: {} entries, loop to {} (header length is {})", positions.GetSize(), positions.GetLoop(),
            uint_t(Source.Length));
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
        // samples are mandatory
        bool hasValidSamples = false;
        bool hasPartialSamples = false;
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Sample result;
          if (const std::size_t samOffset = Source.SamplesOffsets[samIdx])
          {
            const std::size_t availSize = Data.Size() - samOffset;
            if (const auto* src = PeekObject<RawSample>(samOffset))
            {
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
            }
            else
            {
              Dbg("Stub sample {}", samIdx);
            }
          }
          builder.SetSample(samIdx, std::move(result));
        }
        Require(hasValidSamples || hasPartialSamples);
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: {} to parse", ornaments.Count());
        // ornaments are not mandatory
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Ornament result;
          if (const std::size_t ornOffset = Source.OrnamentsOffsets[ornIdx])
          {
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

      template<class T>
      T Peek(std::size_t offset) const
      {
        const auto* data = PeekObject<T>(offset);
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

      bool ParsePattern(uint_t patIndex, std::size_t minOffset, Builder& builder) const
      {
        const RawPattern& pat = GetPattern(patIndex);
        const DataCursors rangesStarts(pat);
        Require(
            rangesStarts.end()
            == std::find_if(rangesStarts.begin(), rangesStarts.end(), Math::NotInRange(minOffset, Data.Size() - 1)));

        ParserState state(rangesStarts);
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
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
          if (state.Offset >= Data.Size() || (0 == chan && 0x00 == PeekByte(state.Offset)))
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
        std::vector<uint_t> commands;
        int_t note = -1;
        while (state.Offset < Data.Size())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd < 0x10)
          {
            commands.push_back(cmd);
          }
          else if ((cmd < 0x20) || (cmd >= 0xb2 && cmd <= 0xbf) || (cmd >= 0xf0))
          {
            const bool hasEnv(cmd >= 0x11 && cmd <= 0xbf);
            const bool hasOrn(cmd >= 0xf0);
            const bool hasSmp(cmd < 0xb2 || cmd > 0xbf);

            if (hasEnv)  // has envelope command
            {
              const uint_t type = cmd - (cmd >= 0xb2 ? 0xb1 : 0x10);
              const uint_t tone = Peek<be_uint16_t>(state.Offset);
              state.Offset += 2;
              builder.SetEnvelope(type, tone);
            }
            else
            {
              builder.SetNoEnvelope();
            }
            if (hasOrn)  // has ornament command
            {
              const uint_t num = cmd - 0xf0;
              builder.SetOrnament(num);
            }
            if (hasSmp)
            {
              const uint_t doubleSampNum = PeekByte(state.Offset++);
              const bool sampValid(doubleSampNum < MAX_SAMPLES_COUNT * 2 && 0 == (doubleSampNum & 1));
              builder.SetSample(sampValid ? (doubleSampNum / 2) : 0);
            }
          }
          else if (cmd < 0x40)
          {
            const uint_t noiseBase = cmd - 0x20;
            builder.SetNoiseBase(noiseBase);
          }
          else if (cmd < 0x50)
          {
            const uint_t num = cmd - 0x40;
            builder.SetOrnament(num);
          }
          else if (cmd < 0xb0)
          {
            note = cmd - 0x50;
            break;
          }
          else if (cmd == 0xb0)
          {
            builder.SetNoEnvelope();
          }
          else if (cmd == 0xb1)
          {
            state.Period = static_cast<uint8_t>(PeekByte(state.Offset++) - 1);
          }
          else if (cmd == 0xc0)
          {
            builder.SetRest();
            break;
          }
          else if (cmd < 0xd0)
          {
            const uint_t val = cmd - 0xc0;
            builder.SetVolume(val);
          }
          else if (cmd == 0xd0)
          {
            break;
          }
          else if (cmd < 0xf0)
          {
            const uint_t num = cmd - 0xd0;
            builder.SetSample(num);
          }
        }
        for (auto it = commands.rbegin(), lim = commands.rend(); it != lim; ++it)
        {
          switch (*it)
          {
          case 1:  // gliss
          {
            const uint_t period = PeekByte(state.Offset++);
            const int_t step = Peek<le_int16_t>(state.Offset);
            state.Offset += 2;
            builder.SetGlissade(period, step);
          }
          break;
          case 2:  // glissnote (portamento)
          {
            const uint_t period = PeekByte(state.Offset++);
            const uint_t limit = Peek<le_uint16_t>(state.Offset);
            state.Offset += 2;
            const int_t step = Peek<le_int16_t>(state.Offset);
            state.Offset += 2;
            builder.SetNoteGliss(period, step, limit);
          }
          break;
          case 3:  // sampleoffset
          {
            const uint_t val = PeekByte(state.Offset++);
            builder.SetSampleOffset(val);
          }
          break;
          case 4:  // ornamentoffset
          {
            const uint_t val = PeekByte(state.Offset++);
            builder.SetOrnamentOffset(val);
          }
          break;
          case 5:  // vibrate
          {
            const uint_t ontime = PeekByte(state.Offset++);
            const uint_t offtime = PeekByte(state.Offset++);
            builder.SetVibrate(ontime, offtime);
          }
          break;
          case 8:  // slide envelope
          {
            const uint_t period = PeekByte(state.Offset++);
            const int_t step = Peek<le_int16_t>(state.Offset);
            state.Offset += 2;
            builder.SetEnvelopeSlide(period, step);
          }
          break;
          case 9:  // tempo
          {
            const uint_t tempo = PeekByte(state.Offset++);
            patBuilder.SetTempo(tempo);
          }
          break;
          }
        }
        if (-1 != note)
        {
          builder.SetNote(note);
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
        dst.Loop = std::min<uint_t>(src.Loop, dst.Lines.size());
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& line)
      {
        Sample::Line res;
        res.Level = line.GetLevel();
        res.VolumeSlideAddon = line.GetVolSlide();
        res.ToneMask = line.GetToneMask();
        res.ToneOffset = line.GetToneOffset();
        res.KeepToneOffset = line.GetKeepToneOffset();
        res.NoiseMask = line.GetNoiseMask();
        res.EnvMask = line.GetEnvelopeMask();
        res.NoiseOrEnvelopeOffset = line.GetNoiseOrEnvelopeOffset();
        res.KeepNoiseOrEnvelopeOffset = line.GetKeepNoiseOrEnvelopeOffset();
        return res;
      }

      static void ParseOrnament(const RawOrnament& src, uint_t size, Ornament& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          dst.Lines[idx] = src.GetLine(idx);
        }
        dst.Loop = std::min<uint_t>(src.Loop, dst.Lines.size());
      }

    private:
      const Binary::View Data;
      RangesMap Ranges;
      const RawHeader& Source;
    };

    bool AddTSPatterns(uint_t patOffset, Indices& target)
    {
      Require(!target.Empty());
      const uint_t patsCount = target.Maximum();
      if (patOffset > patsCount && patOffset < patsCount * 2)
      {
        std::vector<uint_t> mirrored;
        for (Indices::Iterator it = target.Items(); it; ++it)
        {
          mirrored.push_back(patOffset - 1 - *it);
        }
        target.Insert(mirrored.begin(), mirrored.end());
        return true;
      }
      return false;
    }

    enum AreaTypes
    {
      HEADER,
      PATTERNS,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        AddArea(PATTERNS, header.PatternsOffset);
        AddArea(END, size);
      }

      bool CheckHeader(std::size_t headerSize) const
      {
        return GetAreaSize(HEADER) >= headerSize && Undefined == GetAreaSize(END);
      }

      bool CheckPatterns(std::size_t headerSize) const
      {
        return Undefined != GetAreaSize(PATTERNS) && headerSize == GetAreaAddress(PATTERNS);
      }
    };

    Binary::View MakeContainer(Binary::View rawData)
    {
      return rawData.SubView(0, MAX_SIZE);
    }

    bool FastCheck(Binary::View data)
    {
      const std::size_t hdrSize = GetHeaderSize(data);
      if (!Math::InRange<std::size_t>(hdrSize, sizeof(RawHeader) + 1, sizeof(RawHeader) + MAX_POSITIONS_COUNT))
      {
        return false;
      }
      const auto& hdr = *data.As<RawHeader>();
      const Areas areas(hdr, data.Size());
      if (!areas.CheckHeader(hdrSize))
      {
        return false;
      }
      if (!areas.CheckPatterns(hdrSize))
      {
        return false;
      }
      return true;
    }

    const Char DESCRIPTION[] = "Pro Tracker v3.x";
    const auto FORMAT =
        "?{13}"         // uint8_t Id[13];        //'ProTracker 3.'
        "?"             // uint8_t Subversion;
        "?{16}"         // uint8_t Optional1[16]; //' compilation of '
        "?{32}"         // char TrackName[32];
        "?{4}"          // uint8_t Optional2[4]; //' by '
        "?{32}"         // char TrackAuthor[32];
        "?"             // uint8_t Mode;
        "?"             // uint8_t FreqTableNum;
        "01-ff"         // uint8_t Tempo;
        "?"             // uint8_t Length;
        "?"             // uint8_t Loop;
        "?00-01"        // uint16_t PatternsOffset;
        "(?00-bf){32}"  // std::array<uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
        // some of the modules has invalid offsets
        "(?00-d9){16}"  // std::array<uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
        "*3&00-fe"      // at least one position
        "*3"            // next position or limiter (255 % 3 == 0)
        ""_sv;

    class BinaryDecoder : public Decoder
    {
    public:
      BinaryDecoder()
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
        return Formats::Chiptune::ProTracker3::Parse(rawData, stub);
      }

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const override
      {
        return Formats::Chiptune::ProTracker3::Parse(data, target);
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

        StatisticCollectingBuilder statistic(target);
        format.ParseCommonProperties(statistic);
        format.ParsePositions(statistic);
        Indices usedPatterns = statistic.GetUsedPatterns();
        const uint_t mode = statistic.GetMode();
        if (mode != SINGLE_AY_MODE && !AddTSPatterns(mode, usedPatterns))
        {
          target.SetMode(SINGLE_AY_MODE);
        }
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

    Decoder::Ptr CreateDecoder()
    {
      return MakePtr<BinaryDecoder>();
    }
  }  // namespace ProTracker3

  Decoder::Ptr CreateProTracker3Decoder()
  {
    return ProTracker3::CreateDecoder();
  }
}  // namespace Formats::Chiptune
