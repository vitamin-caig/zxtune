/*
Abstract:
  SoundTracker format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtracker_detail.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
//library includes
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::SoundTracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace SoundTrackerUncompiled
  {
    using namespace SoundTracker;
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawSample
    {
      uint8_t Volume[SAMPLE_SIZE];
      uint8_t Noise[SAMPLE_SIZE];
      uint16_t Effect[SAMPLE_SIZE];
      uint8_t Loop;
      uint8_t LoopSize;
    } PACK_POST;

    PACK_PRE struct RawPosEntry
    {
      uint8_t Pattern;
      int8_t Transposition;
    } PACK_POST;

    PACK_PRE struct RawOrnament
    {
      boost::array<int8_t, ORNAMENT_SIZE> Offsets;
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      PACK_PRE struct Line
      {
        PACK_PRE struct Channel
        {
          //RNNN#OOO
          uint8_t Note;
          //SSSSEEEE
          uint8_t EffectSample;
          //EEEEeeee
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
        } PACK_POST;

        Channel Channels[3];

        bool IsEmpty() const
        {
          return Channels[0].IsEmpty() && Channels[1].IsEmpty() && Channels[2].IsEmpty();
        }
      } PACK_POST;

      Line Lines[MAX_PATTERN_SIZE];
    } PACK_POST;

    PACK_PRE struct RawHeader
    {
      RawSample Samples[MAX_SAMPLES_COUNT - 1];
      RawPosEntry Positions[MAX_POSITIONS_COUNT];
      uint8_t Length;
      RawOrnament Ornaments[MAX_ORNAMENTS_COUNT + 1];
      uint8_t Tempo;
      uint8_t PatternsSize;
      //at least one pattern
      RawPattern Patterns[1];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawSample) == 130);
    BOOST_STATIC_ASSERT(sizeof(RawPosEntry) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 32);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 576);
    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 3009 + 576);
    BOOST_STATIC_ASSERT(offsetof(RawHeader, Positions) == 1950);
    BOOST_STATIC_ASSERT(offsetof(RawHeader, Length) == 2462);
    BOOST_STATIC_ASSERT(offsetof(RawHeader, Tempo) == 3007);
    BOOST_STATIC_ASSERT(offsetof(RawHeader, Patterns) == 3009);

    const std::size_t MIN_SIZE = sizeof(RawHeader);

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Delegate(data)
        , Source(*Delegate.GetField<RawHeader>(0))
        , MaxPatterns(1 + (data.Size() - sizeof(Source)) / sizeof(RawPattern))
      {
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Text::SOUNDTRACKER_DECODER_DESCRIPTION);
      }

      void ParsePositions(Builder& builder) const
      {
        const std::size_t posCount = Source.Length + 1;
        std::vector<PositionEntry> positions(posCount);
        for (uint_t idx = 0; idx != posCount; ++idx)
        {
          const RawPosEntry& src = Source.Positions[idx];
          PositionEntry& dst = positions[idx];
          dst.PatternIndex = src.Pattern - 1;
          dst.Transposition = src.Transposition;
        }
        builder.SetPositions(positions);
        Dbg("Positions: %1% entries", positions.size());
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          if (patIndex < MaxPatterns)
          {
            Dbg("Parse pattern %1%", patIndex);
            ParsePattern(patIndex, builder);
          }
          else
          {
            Dbg("Fill stub pattern %1%", patIndex);
            builder.StartPattern(patIndex).Finish(Source.PatternsSize);
          }
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Dbg("Parse sample %1%", samIdx);
          if (samIdx)
          {
            Sample result;
            ParseSample(Source.Samples[samIdx - 1], result);
            builder.SetSample(samIdx, result);
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
          Dbg("Parse ornament %1%", ornIdx);
          const RawOrnament& src = Source.Ornaments[ornIdx];
          const Ornament result(src.Offsets.begin(), src.Offsets.end());
          builder.SetOrnament(ornIdx, result);
        }
      }

      uint_t GetMaxPatterns() const
      {
        return MaxPatterns;
      }
    private:
      const RawPattern& GetPattern(uint_t index) const
      {
        const RawPattern* const src = Delegate.GetField<RawPattern>(offsetof(RawHeader, Patterns) + index * sizeof(RawPattern));
        Require(src != 0);
        return *src;
      }

      struct EnvState
      {
        uint_t Type;
        uint_t Tone;

        EnvState()
          : Type(), Tone()
        {
        }
      };

      struct ChanState
      {
        uint_t Sample;
        uint_t Ornament;

        ChanState()
          : Sample(), Ornament()
        {
        }
      };

      void ParsePattern(uint_t patIndex, Builder& builder) const
      {
        const RawPattern& src = GetPattern(patIndex);
        const std::size_t patSize = Source.PatternsSize;
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        boost::array<EnvState, 3> env;
        boost::array<ChanState, 3> state;
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

      static void ParseLine(const RawPattern::Line& srcLine, Builder& builder, boost::array<ChanState, 3>& state, boost::array<EnvState, 3>& env)
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

      static void ParseChannel(const RawPattern::Line::Channel& srcChan, Builder& builder, ChanState& state, EnvState& env)
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
        //default:
          env.Type = 0;
          break;
        }
      }

      static uint_t ConvertNote(uint8_t note)
      {
        static const uint_t HALFTONES[] = 
        {
          ~uint_t(0), //invalid
          ~uint_t(0), //invalid#
          9,  //A
          10, //A#
          11, //B
          ~uint_t(0), //B#,invalid
          0,  //C
          1,  //C#
          2,  //D
          3,  //D#
          4,  //E
          ~uint_t(0), //E#,invalid
          5,  //F
          6,  //F#
          7,  //G
          8,  //G#
        };

        const uint_t NOTES_PER_OCTAVE = 12;
        const uint_t octave = note & 7;
        const uint_t halftone = (note & 0x78) >> 3;
        return HALFTONES[halftone] + NOTES_PER_OCTAVE * octave;
      }

      static void ParseSample(const RawSample& src, Sample& dst)
      {
        dst.Lines.resize(SAMPLE_SIZE);
        for (uint_t idx = 0; idx < SAMPLE_SIZE; ++idx)
        {
          Sample::Line& res = dst.Lines[idx];
          res.Level = src.Volume[idx];
          res.Noise = src.Noise[idx] & 31;
          res.NoiseMask = 0 != (src.Noise[idx] & 128);
          res.EnvelopeMask = 0 != (src.Noise[idx] & 64);
          const int16_t eff = fromLE(src.Effect[idx]);
          res.Effect = 0 != (eff & 0x1000) ? (eff & 0xfff) : -(eff & 0xfff);
        }
        dst.Loop = std::min<uint_t>(src.Loop, SAMPLE_SIZE);
        dst.LoopLimit = std::min<uint_t>(src.Loop + src.LoopSize + 1, SAMPLE_SIZE);
      }
    private:
      const Binary::TypedContainer Delegate;
      const RawHeader& Source;
      const uint_t MaxPatterns;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      //at least
      return rawData.Size() >= sizeof(RawHeader);
    }

    const std::string FORMAT(
      //samples
      "("
        //levels
        "00-0f{32}"
        //noises. Bit 5 should be empty
        "%xx0xxxxx{32}"
        //additions
        "(?00-1f){32}"
        //loop, loop limit
        "00-1f{2}"
      "){15}"
      //positions
      "(01-20?){256}"
      //length
      "00-7f"
      //ornaments
      "(?{32}){17}"
      //delay
      //Real delay may be from 01 but I don't know any module with such little delay
      "02-0f"
      //patterns size
      //Real pattern size may be from 01 but I don't know any modules with such patterns size
      "20-40"
    );

    Formats::Chiptune::Container::Ptr ParseUncompiled(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
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
        const Binary::Container::Ptr subData = data.GetSubcontainer(0, size);
        const std::size_t patternsOffset = offsetof(RawHeader, Patterns);
        return CreateCalculatingCrcContainer(subData, patternsOffset, size - patternsOffset);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }

    class Decoder : public Formats::Chiptune::SoundTracker::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SOUNDTRACKER_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return FastCheck(rawData) && Format->Match(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (!Format->Match(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        Builder& stub = GetStubBuilder();
        return ParseUncompiled(rawData, stub);
      }

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const
      {
        return ParseUncompiled(data, target);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//namespace SoundTrackerUncompiled

  namespace SoundTracker
  {
    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, const Sample& /*sample*/) {}
      virtual void SetOrnament(uint_t /*index*/, const Ornament& /*ornament*/) {}
      virtual void SetPositions(const std::vector<PositionEntry>& /*positions*/) {}
      virtual PatternBuilder& StartPattern(uint_t /*index*/)
      {
        return GetStubPatternBuilder();
      }
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
      virtual void SetNoEnvelope() {}
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
        return boost::make_shared<SoundTrackerUncompiled::Decoder>();
      }
    }
  }

  Formats::Chiptune::Decoder::Ptr CreateSoundTrackerDecoder()
  {
    return SoundTracker::Ver1::CreateUncompiledDecoder();
  }
}// namespace Chiptune
}// namespace Formats
