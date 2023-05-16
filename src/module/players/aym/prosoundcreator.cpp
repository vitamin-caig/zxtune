/**
 *
 * @file
 *
 * @brief  ProSoundCreator chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/prosoundcreator.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
// common includes
#include <make_ptr.h>
// library includes
#include <formats/chiptune/aym/prosoundcreator.h>
#include <math/numeric.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>

namespace Module::ProSoundCreator
{
  /*
    Do not use GLISS_NOTE command due to possible ambiguation while parsing
  */
  // supported commands and parameters
  enum CmdType
  {
    // no parameters
    EMPTY,
    // r13,period or nothing
    ENVELOPE,
    // no parameters
    NOENVELOPE,
    // noise base
    NOISE_BASE,
    // no params
    BREAK_SAMPLE,
    // no params
    BREAK_ORNAMENT,
    // no params
    NO_ORNAMENT,
    // absolute gliss
    GLISS,
    // step
    SLIDE,
    // period, delta
    VOLUME_SLIDE
  };

  using Formats::Chiptune::ProSoundCreator::Sample;
  using Formats::Chiptune::ProSoundCreator::Ornament;

  template<class Object>
  class ObjectLinesIterator
  {
  public:
    ObjectLinesIterator()
      : Obj()
    {}

    void Set(const Object& obj)
    {
      // do not reset for original position update algo
      Obj = &obj;
    }

    void Reset()
    {
      Position = 0;
      Break = false;
    }

    void Disable()
    {
      Position = Obj->GetSize();
    }

    const typename Object::Line* GetLine() const
    {
      return Position < Obj->GetSize() ? &Obj->GetLine(Position) : nullptr;
    }

    void SetBreakLoop(bool brk)
    {
      Break = brk;
    }

    void Next()
    {
      const typename Object::Line& curLine = Obj->GetLine(Position);
      if (curLine.LoopBegin)
      {
        LoopPosition = Position;
      }
      if (curLine.LoopEnd)
      {
        if (!Break)
        {
          Position = LoopPosition;
        }
        else
        {
          Break = false;
          ++Position;
        }
      }
      else
      {
        ++Position;
      }
    }

  private:
    const Object* Obj;
    uint_t Position = 0;
    uint_t LoopPosition = 0;
    bool Break = false;
  };

  class ToneSlider
  {
  public:
    ToneSlider() = default;

    void SetSliding(int_t sliding)
    {
      Sliding = sliding;
    }

    void SetSlidingSteps(uint_t steps)
    {
      Steps = steps;
    }

    void SetGlissade(int_t glissade)
    {
      Glissade = glissade;
    }

    void Reset()
    {
      Glissade = 0;
    }

    int_t GetSliding() const
    {
      return Sliding;
    }

    int_t Update()
    {
      if (Glissade)
      {
        Sliding += Glissade;
        if (Steps && !--Steps)
        {
          Glissade = 0;
        }
        return Sliding;
      }
      else
      {
        return 0;
      }
    }

  private:
    int_t Sliding = 0;
    int_t Glissade = 0;
    uint_t Steps = 0;
  };

  class VolumeSlider
  {
  public:
    VolumeSlider() = default;

    void Reset()
    {
      Counter = 0;
    }

    void SetParams(uint_t period, int_t delta)
    {
      Period = Counter = period;
      Delta = delta;
    }

    int_t Update()
    {
      if (Counter && !--Counter)
      {
        Counter = Period;
        return Delta;
      }
      else
      {
        return 0;
      }
    }

  private:
    uint_t Counter = 0;
    uint_t Period = 0;
    int_t Delta = 0;
  };

  using ModuleData = AYM::ModuleData<OrderList, Sample, Ornament>;

  class DataBuilder : public Formats::Chiptune::ProSoundCreator::Builder
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(TABLE_ASM);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::ProSoundCreator::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::ProSoundCreator::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::ProSoundCreator::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
    }

    Formats::Chiptune::PatternBuilder& StartPattern(uint_t index) override
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    void StartChannel(uint_t index) override
    {
      Patterns.SetChannel(index);
    }

    void SetRest() override
    {
      Patterns.GetChannel().SetEnabled(false);
    }

    void SetNote(uint_t note) override
    {
      MutableCell& channel = Patterns.GetChannel();
      channel.SetEnabled(true);
      channel.SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      Patterns.GetChannel().SetSample(sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    void SetVolume(uint_t vol) override
    {
      Patterns.GetChannel().SetVolume(vol);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    void SetEnvelope() override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(NOENVELOPE);
    }

    void SetNoiseBase(uint_t val) override
    {
      Patterns.GetChannel().AddCommand(NOISE_BASE, val);
    }

    void SetBreakSample() override
    {
      Patterns.GetChannel().AddCommand(BREAK_SAMPLE);
    }

    void SetBreakOrnament() override
    {
      Patterns.GetChannel().AddCommand(BREAK_ORNAMENT);
    }

    void SetNoOrnament() override
    {
      Patterns.GetChannel().AddCommand(NO_ORNAMENT);
    }

    void SetGliss(uint_t absStep) override
    {
      Patterns.GetChannel().AddCommand(GLISS, absStep);
    }

    void SetSlide(int_t delta) override
    {
      Patterns.GetChannel().AddCommand(SLIDE, delta);
    }

    void SetVolumeSlide(uint_t period, int_t delta) override
    {
      Patterns.GetChannel().AddCommand(VOLUME_SLIDE, period, delta);
    }

    ModuleData::Ptr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }

  private:
    AYM::PropertiesHelper& Properties;
    MetaProperties Meta;
    PatternsBuilder Patterns;
    ModuleData::RWPtr Data;
  };

  struct ChannelState
  {
    ChannelState() = default;

    ObjectLinesIterator<Sample> SampleIterator;
    ObjectLinesIterator<Ornament> OrnamentIterator;
    ToneSlider ToneSlide;
    VolumeSlider VolumeSlide;
    int_t Note = 0;
    int16_t ToneAccumulator = 0;
    uint_t Volume = 0;
    int_t Attenuation = 0;
    uint_t NoiseAccumulator = 0;
    bool EnvelopeEnabled = false;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
      , EnvelopeTone()
      , NoiseBase()
    {
      Reset();
    }

    void Reset() override
    {
      const Sample& stubSample = Data->Samples.Get(0);
      const Ornament& stubOrnament = Data->Ornaments.Get(0);
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        ChannelState& dst = PlayerState[chan];
        dst = ChannelState();
        dst.SampleIterator.Set(stubSample);
        dst.SampleIterator.SetBreakLoop(false);
        dst.SampleIterator.Disable();
        dst.OrnamentIterator.Set(stubOrnament);
        dst.OrnamentIterator.SetBreakLoop(false);
        dst.OrnamentIterator.Disable();
      }
      EnvelopeTone = 0;
      NoiseBase = 0;
    }

    void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track) override
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }

  private:
    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (const auto line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const auto src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        PlayerState[chan].NoiseAccumulator += NoiseBase;
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      const auto oldTone =
          static_cast<uint16_t>(track.GetFrequency(dst.Note) + dst.ToneAccumulator + dst.ToneSlide.GetSliding());
      if (const bool* enabled = src.GetEnabled())
      {
        if (*enabled)
        {
          dst.SampleIterator.Reset();
          dst.OrnamentIterator.Reset();
          dst.VolumeSlide.Reset();
          dst.ToneSlide.SetSliding(0);
          dst.ToneAccumulator = 0;
          dst.NoiseAccumulator = 0;
          dst.Attenuation = 0;
        }
        else
        {
          dst.SampleIterator.SetBreakLoop(false);
          dst.SampleIterator.Disable();
          dst.OrnamentIterator.SetBreakLoop(false);
          dst.OrnamentIterator.Disable();
        }
        dst.ToneSlide.Reset();
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleIterator.Set(Data->Samples.Get(*sample));
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentIterator.Set(Data->Ornaments.Get(*ornament));
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
        dst.Attenuation = 0;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case BREAK_SAMPLE:
          dst.SampleIterator.SetBreakLoop(true);
          break;
        case BREAK_ORNAMENT:
          dst.OrnamentIterator.SetBreakLoop(true);
          break;
        case NO_ORNAMENT:
          dst.OrnamentIterator.Disable();
          break;
        case ENVELOPE:
          if (it->Param1 || it->Param2)
          {
            track.SetEnvelopeType(it->Param1);
            track.SetEnvelopeTone(EnvelopeTone = it->Param2);
          }
          else
          {
            dst.EnvelopeEnabled = true;
          }
          break;
        case NOENVELOPE:
          dst.EnvelopeEnabled = false;
          break;
        case NOISE_BASE:
          NoiseBase = it->Param1;
          break;
        case GLISS:
        {
          const int_t sliding = oldTone - track.GetFrequency(dst.Note);
          const int_t newGliss = sliding >= 0 ? -it->Param1 : it->Param1;
          const int_t steps = 0 != it->Param1 ? (1 + Math::Absolute(sliding) / it->Param1) : 0;
          dst.ToneSlide.SetSliding(sliding);
          dst.ToneSlide.SetGlissade(newGliss);
          dst.ToneSlide.SetSlidingSteps(steps);
        }
        break;
        case SLIDE:
          dst.ToneSlide.SetGlissade(it->Param1);
          dst.ToneSlide.SetSlidingSteps(0);
          break;
        case VOLUME_SLIDE:
          dst.VolumeSlide.SetParams(it->Param1, it->Param2);
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      const Sample::Line* const curSampleLine = dst.SampleIterator.GetLine();
      if (!curSampleLine)
      {
        channel.SetLevel(0);
        return;
      }
      dst.SampleIterator.Next();

      // apply tone
      if (const Ornament::Line* curOrnamentLine = dst.OrnamentIterator.GetLine())
      {
        dst.NoiseAccumulator += curOrnamentLine->NoiseAddon;
        dst.Note += curOrnamentLine->NoteAddon;
        if (dst.Note < 0)
        {
          dst.Note += 0x56;
        }
        else if (dst.Note > 0x55)
        {
          dst.Note -= 0x56;
        }
        if (dst.Note > 0x55)
        {
          dst.Note = 0x55;
        }
        dst.OrnamentIterator.Next();
      }
      dst.ToneAccumulator += curSampleLine->Tone;
      const int_t toneOffset = dst.ToneAccumulator + dst.ToneSlide.Update();
      channel.SetTone(dst.Note, toneOffset);
      if (curSampleLine->ToneMask)
      {
        channel.DisableTone();
      }

      // apply level
      dst.Attenuation += curSampleLine->VolumeDelta + dst.VolumeSlide.Update();
      const auto level = Math::Clamp<int_t>(dst.Attenuation + dst.Volume, 0, 15);
      dst.Attenuation = level - dst.Volume;
      const uint_t vol = 1 + static_cast<uint_t>(level);
      channel.SetLevel(vol * curSampleLine->Level >> 4);

      // apply noise+envelope
      const bool envelope = dst.EnvelopeEnabled && curSampleLine->EnableEnvelope;
      if (envelope)
      {
        channel.EnableEnvelope();
      }
      if (envelope && curSampleLine->NoiseMask)
      {
        EnvelopeTone += curSampleLine->Adding;
        track.SetEnvelopeTone(EnvelopeTone);
      }
      else
      {
        dst.NoiseAccumulator += curSampleLine->Adding;
        if (!curSampleLine->NoiseMask)
        {
          track.SetNoise(dst.NoiseAccumulator);
        }
      }
      if (curSampleLine->NoiseMask)
      {
        channel.DisableNoise();
      }
    }

  private:
    const ModuleData::Ptr Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
    uint_t EnvelopeTone;
    uint_t NoiseBase;
  };

  class Factory : public AYM::Factory
  {
  public:
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Formats::Chiptune::ProSoundCreator::Parse(rawData, dataBuilder))
      {
        props.SetSource(*container);
        props.SetPlatform(Platforms::ZX_SPECTRUM);
        return MakePtr<AYM::TrackingChiptune<ModuleData, DataRenderer>>(dataBuilder.CaptureResult(),
                                                                        std::move(properties));
      }
      else
      {
        return {};
      }
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::ProSoundCreator
