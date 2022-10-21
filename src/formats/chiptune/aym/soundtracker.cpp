/**
 *
 * @file
 *
 * @brief  SoundTracker uncompiled modules support implementation
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
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <math/numeric.h>
// std includes
#include <array>

namespace Formats::Chiptune
{
  namespace SoundTrackerUncompiled
  {
    const Debug::Stream Dbg("Formats::Chiptune::SoundTracker");

    const Char PROGRAM[] = "Sound Tracker v1.x";

    using namespace SoundTracker;

    struct RawSample
    {
      uint8_t Volume[SAMPLE_SIZE];
      uint8_t Noise[SAMPLE_SIZE];
      le_uint16_t Effect[SAMPLE_SIZE];
      uint8_t Loop;
      uint8_t LoopSize;
    };

    struct RawPosEntry
    {
      uint8_t Pattern;
      int8_t Transposition;
    };

    struct RawOrnament
    {
      std::array<int8_t, ORNAMENT_SIZE> Offsets;
    };

    struct RawPattern
    {
      struct Line
      {
        struct Channel
        {
          // RNNN#OOO
          uint8_t Note;
          // SSSSEEEE
          uint8_t EffectSample;
          // EEEEeeee
          uint8_t EffectOrnament;

          bool IsEmpty() const
          {
            return !IsRest() && !HasNote() && 0 == EffectSample;
          }

          bool IsRest() const
          {
            return 0 != (Note & 128);
          }

          bool HasNote() const
          {
            return 0 != (Note & 0x78);
          }

          uint_t GetEffect() const
          {
            return EffectSample & 15;
          }

          uint_t GetEffectParam() const
          {
            return EffectOrnament;
          }

          uint_t GetSample() const
          {
            return EffectSample >> 4;
          }

          uint_t GetOrnament() const
          {
            return EffectOrnament & 15;
          }
        };

        Channel Channels[3];

        bool IsEmpty() const
        {
          return Channels[0].IsEmpty() && Channels[1].IsEmpty() && Channels[2].IsEmpty();
        }
      };

      Line Lines[MAX_PATTERN_SIZE];
    };

    struct RawHeader
    {
      RawSample Samples[MAX_SAMPLES_COUNT - 1];
      RawPosEntry Positions[MAX_POSITIONS_COUNT];
      uint8_t Length;
      RawOrnament Ornaments[MAX_ORNAMENTS_COUNT + 1];
      uint8_t Tempo;
      uint8_t PatternsSize;
      // at least one pattern
      RawPattern Patterns[1];
    };

    static_assert(sizeof(RawSample) * alignof(RawSample) == 130, "Invalid layout");
    static_assert(sizeof(RawPosEntry) * alignof(RawPosEntry) == 2, "Invalid layout");
    static_assert(sizeof(RawOrnament) * alignof(RawOrnament) == 32, "Invalid layout");
    static_assert(sizeof(RawPattern) * alignof(RawPattern) == 576, "Invalid layout");
    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 3009 + 576, "Invalid layout");
    static_assert(offsetof(RawHeader, Positions) == 1950, "Invalid layout");
    static_assert(offsetof(RawHeader, Length) == 2462, "Invalid layout");
    static_assert(offsetof(RawHeader, Tempo) == 3007, "Invalid layout");
    static_assert(offsetof(RawHeader, Patterns) == 3009, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(RawHeader);

    class Format
    {
    public:
      explicit Format(Binary::View data)
        : Data(data)
        , Source(*Data.As<RawHeader>())
        , MaxPatterns(1 + (data.Size() - sizeof(Source)) / sizeof(RawPattern))
      {}

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(PROGRAM);
      }

      void ParsePositions(Builder& builder) const
      {
        const std::size_t posCount = Source.Length + 1;
        Positions positions;
        positions.Lines.resize(posCount);
        for (uint_t idx = 0; idx != posCount; ++idx)
        {
          const RawPosEntry& src = Source.Positions[idx];
          PositionEntry& dst = positions.Lines[idx];
          dst.PatternIndex = src.Pattern - 1;
          dst.Transposition = src.Transposition;
        }
        Dbg("Positions: {} entries", positions.GetSize());
        builder.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          if (patIndex < MaxPatterns)
          {
            Dbg("Parse pattern {}", patIndex);
            ParsePattern(patIndex, builder);
          }
          else
          {
            Dbg("Fill stub pattern {}", patIndex);
            builder.StartPattern(patIndex).Finish(Source.PatternsSize);
          }
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Dbg("Parse sample {}", samIdx);
          if (samIdx)
          {
            builder.SetSample(samIdx, ParseSample(Source.Samples[samIdx - 1]));
          }
          else
          {
            builder.SetSample(samIdx, Sample());
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
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Dbg("Parse ornament {}", ornIdx);
          builder.SetOrnament(ornIdx, ParseOrnament(Source.Ornaments[ornIdx]));
        }
      }

      uint_t GetMaxPatterns() const
      {
        return MaxPatterns;
      }

    private:
      const RawPattern& GetPattern(uint_t index) const
      {
        const auto* src = Data.SubView(offsetof(RawHeader, Patterns) + index * sizeof(RawPattern)).As<RawPattern>();
        Require(src != nullptr);
        return *src;
      }

      struct EnvState
      {
        uint_t Type;
        uint_t Tone;

        EnvState()
          : Type()
          , Tone()
        {}
      };

      struct ChanState
      {
        uint_t Sample;
        uint_t Ornament;

        ChanState()
          : Sample()
          , Ornament()
        {}
      };

      void ParsePattern(uint_t patIndex, Builder& builder) const
      {
        const RawPattern& src = GetPattern(patIndex);
        const std::size_t patSize = Source.PatternsSize;
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        std::array<EnvState, 3> env;
        std::array<ChanState, 3> state;
        for (uint_t idx = 0; idx < MAX_PATTERN_SIZE; ++idx)
        {
          const RawPattern::Line& srcLine = src.Lines[idx];
          if (srcLine.IsEmpty())
          {
            continue;
          }
          Require(idx < patSize);
          patBuilder.StartLine(idx);
          ParseLine(srcLine, builder, state, env);
        }
        patBuilder.Finish(patSize);
      }

      static void ParseLine(const RawPattern::Line& srcLine, Builder& builder, std::array<ChanState, 3>& state,
                            std::array<EnvState, 3>& env)
      {
        for (uint_t chan = 0; chan < state.size(); ++chan)
        {
          if (srcLine.Channels[chan].IsEmpty())
          {
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(srcLine.Channels[chan], builder, state[chan], env[chan]);
        }
      }

      static void ParseChannel(const RawPattern::Line::Channel& srcChan, Builder& builder, ChanState& state,
                               EnvState& env)
      {
        if (srcChan.IsRest())
        {
          builder.SetRest();
          return;
        }
        else if (!srcChan.HasNote())
        {
          return;
        }
        builder.SetNote(ConvertNote(srcChan.Note));
        {
          const uint_t sample = srcChan.GetSample();
          if (sample && sample != state.Sample)
          {
            builder.SetSample(sample);
          }
          state.Sample = sample;
        }
        switch (const uint_t effect = srcChan.GetEffect())
        {
        case 15:
        {
          const uint_t ornament = srcChan.GetOrnament();
          builder.SetOrnament(ornament);
          state.Ornament = ornament;
          builder.SetNoEnvelope();
          env.Type = 0;
        }
        break;
        case 8:
        case 10:
        case 12:
        case 13:
        case 14:
        {
          const uint_t tone = srcChan.GetEffectParam();
          if (effect != env.Type || tone != env.Tone)
          {
            env.Type = effect;
            env.Tone = tone;
            builder.SetOrnament(0);
            builder.SetEnvelope(env.Type, env.Tone);
            state.Ornament = 0;
          }
        }
        break;
        case 1:
          builder.SetNoEnvelope();
          builder.SetOrnament(0);
          // default:
          env.Type = 0;
          break;
        }
      }

      static uint_t ConvertNote(uint8_t note)
      {
        static const uint_t HALFTONES[] = {
            ~uint_t(0),  // invalid
            ~uint_t(0),  // invalid#
            9,           // A
            10,          // A#
            11,          // B
            ~uint_t(0),  // B#,invalid
            0,           // C
            1,           // C#
            2,           // D
            3,           // D#
            4,           // E
            ~uint_t(0),  // E#,invalid
            5,           // F
            6,           // F#
            7,           // G
            8,           // G#
        };

        const uint_t NOTES_PER_OCTAVE = 12;
        const uint_t octave = note & 7;
        const uint_t halftone = (note & 0x78) >> 3;
        return HALFTONES[halftone] + NOTES_PER_OCTAVE * octave;
      }

      static Sample ParseSample(const RawSample& src)
      {
        Sample dst;
        dst.Lines.resize(SAMPLE_SIZE);
        for (uint_t idx = 0; idx < SAMPLE_SIZE; ++idx)
        {
          Sample::Line& res = dst.Lines[idx];
          res.Level = src.Volume[idx];
          res.Noise = src.Noise[idx] & 31;
          res.NoiseMask = 0 != (src.Noise[idx] & 128);
          res.EnvelopeMask = 0 != (src.Noise[idx] & 64);
          const int16_t eff = src.Effect[idx];
          res.Effect = 0 != (eff & 0x1000) ? (eff & 0xfff) : -(eff & 0xfff);
        }
        dst.Loop = std::min<uint_t>(src.Loop, SAMPLE_SIZE);
        dst.LoopLimit = std::min<uint_t>(src.Loop + src.LoopSize, SAMPLE_SIZE);
        return dst;
      }

      static Ornament ParseOrnament(const RawOrnament& src)
      {
        Ornament dst;
        dst.Lines.assign(src.Offsets.begin(), src.Offsets.end());
        return dst;
      }

    private:
      const Binary::View Data;
      const RawHeader& Source;
      const uint_t MaxPatterns;
    };

    bool FastCheck(Binary::View rawData)
    {
      // at least
      return rawData.Size() >= sizeof(RawHeader);
    }

    const auto DESCRIPTION = PROGRAM;
    const auto FORMAT =
        // samples
        "("
        // levels
        "00-0f{32}"
        // noises. Bit 5 should be empty
        "%xx0xxxxx{32}"
        // additions
        "(?00-1f){32}"
        // loop, loop limit
        "00-1f{2}"
        "){15}"
        // positions
        "(01-20?){256}"
        // length
        "00-7f"
        // ornaments
        "(?{32}){17}"
        // delay
        // Real delay may be from 01 but I don't know any module with such little delay
        "02-0f"
        // patterns size
        // Real pattern size may be from 01 but I don't know any modules with such patterns size
        "20-40"
        ""_sv;

    Formats::Chiptune::Container::Ptr ParseUncompiled(const Binary::Container& rawData, Builder& target)
    {
      const Binary::View data(rawData);
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

        const uint_t lastPattern = std::min(usedPatterns.Maximum(), format.GetMaxPatterns() - 1);
        const std::size_t size = sizeof(RawHeader) + lastPattern * sizeof(RawPattern);
        auto subData = rawData.GetSubcontainer(0, size);
        const std::size_t patternsOffset = offsetof(RawHeader, Patterns);
        return CreateCalculatingCrcContainer(std::move(subData), patternsOffset, size - patternsOffset);
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

      bool Check(Binary::View data) const override
      {
        return FastCheck(data) && Format->Match(data);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return {};
        }
        Builder& stub = GetStubBuilder();
        return ParseUncompiled(rawData, stub);
      }

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const override
      {
        return ParseUncompiled(data, target);
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace SoundTrackerUncompiled

  namespace SoundTracker
  {
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
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    namespace Ver1
    {
      Decoder::Ptr CreateUncompiledDecoder()
      {
        return MakePtr<SoundTrackerUncompiled::Decoder>();
      }
    }  // namespace Ver1
  }    // namespace SoundTracker

  Formats::Chiptune::Decoder::Ptr CreateSoundTrackerDecoder()
  {
    return SoundTracker::Ver1::CreateUncompiledDecoder();
  }
}  // namespace Formats::Chiptune
