/**
 *
 * @file
 *
 * @brief  ASCSoundMaster chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/ascsoundmaster.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_properties_helper.h"
// common includes
#include <make_ptr.h>
// library includes
#include <formats/chiptune/aym/ascsoundmaster.h>
#include <math/numeric.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/simple_orderlist.h>
// std includes
#include <array>

namespace Module::ASCSoundMaster
{
  // supported commands and their parameters
  enum CmdType
  {
    // no parameters
    EMPTY,
    // r13,envL
    ENVELOPE,
    // no parameters
    ENVELOPE_ON,
    // no parameters
    ENVELOPE_OFF,
    // value
    NOISE,
    // no parameters
    CONT_SAMPLE,
    // no parameters
    CONT_ORNAMENT,
    // glissade value
    GLISS,
    // steps
    SLIDE,
    // steps,note
    SLIDE_NOTE,
    // period,delta
    AMPLITUDE_SLIDE,
    // no parameter
    BREAK_SAMPLE
  };

  using Formats::Chiptune::ASCSoundMaster::Sample;
  using Formats::Chiptune::ASCSoundMaster::Ornament;

  using ModuleData = AYM::ModuleData<OrderList, Sample, Ornament>;

  class DataBuilder : public Formats::Chiptune::ASCSoundMaster::Builder
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

    void SetSample(uint_t index, Formats::Chiptune::ASCSoundMaster::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::ASCSoundMaster::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::ASCSoundMaster::Positions positions) override
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
      if (!Patterns.GetChannel().FindCommand(BREAK_SAMPLE))
      {
        Patterns.GetChannel().SetEnabled(true);
      }
      if (Command* cmd = Patterns.GetChannel().FindCommand(SLIDE))
      {
        // set slide to note
        cmd->Type = SLIDE_NOTE;
        cmd->Param3 = cmd->Param2;
        cmd->Param2 = note;
      }
      else
      {
        Patterns.GetChannel().SetNote(note);
      }
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

    void SetEnvelopeType(uint_t type) override
    {
      if (Command* cmd = Patterns.GetChannel().FindCommand(ENVELOPE))
      {
        cmd->Param1 = int_t(type);
      }
      else
      {
        Patterns.GetChannel().AddCommand(ENVELOPE, int_t(type), -1);
      }
    }

    void SetEnvelopeTone(uint_t tone) override
    {
      if (Command* cmd = Patterns.GetChannel().FindCommand(ENVELOPE))
      {
        cmd->Param2 = int_t(tone);
      }
      else
      {
        // strange situation
        Patterns.GetChannel().AddCommand(ENVELOPE, -1, int_t(tone));
      }
    }

    void SetEnvelope() override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE_ON);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().AddCommand(ENVELOPE_OFF);
    }

    void SetNoise(uint_t val) override
    {
      Patterns.GetChannel().AddCommand(NOISE, val);
    }

    void SetContinueSample() override
    {
      Patterns.GetChannel().AddCommand(CONT_SAMPLE);
    }

    void SetContinueOrnament() override
    {
      Patterns.GetChannel().AddCommand(CONT_ORNAMENT);
    }

    void SetGlissade(int_t val) override
    {
      Patterns.GetChannel().AddCommand(GLISS, val);
    }

    void SetSlide(int_t steps, bool useToneSliding) override
    {
      Patterns.GetChannel().AddCommand(SLIDE, steps, useToneSliding);
    }

    void SetVolumeSlide(uint_t period, int_t delta) override
    {
      Patterns.GetChannel().AddCommand(AMPLITUDE_SLIDE, period, delta);
    }

    void SetBreakSample() override
    {
      Patterns.GetChannel().AddCommand(BREAK_SAMPLE);
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

  const uint_t LIMITER(~uint_t(0));

  struct ChannelState
  {
    ChannelState()
      : SlidingTargetNote(LIMITER)
    {}
    bool Enabled = false;
    bool Envelope = false;
    bool BreakSample = false;
    uint_t Volume = 15;
    int_t VolumeAddon = 0;
    uint_t VolSlideDelay = 0;
    int_t VolSlideAddon = 0;
    uint_t VolSlideCounter = 0;
    int_t BaseNoise = 0;
    int_t CurrentNoise = 0;
    uint_t Note = 0;
    int8_t NoteAddon = 0;
    uint_t SampleNum = 0;
    uint_t CurrentSampleNum = 0;
    uint_t PosInSample = 0;
    uint_t OrnamentNum = 0;
    uint_t CurrentOrnamentNum = 0;
    uint_t PosInOrnament = 0;
    int_t ToneDeviation = 0;
    int_t SlidingSteps = 0;  // may be infinite (negative)
    int_t Sliding = 0;
    uint_t SlidingTargetNote;
    int_t Glissade = 0;

    void ResetBaseNoise()
    {
      BaseNoise = 0;
    }
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
    {}

    void Reset() override
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
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
      if (0 == state.Line())
      {
        for (auto& state : PlayerState)
        {
          state.ResetBaseNoise();
        }
      }
      if (const auto* const line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const auto* const src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
      }
      dst.VolSlideCounter = 0;
      dst.SlidingSteps = 0;
      bool contSample = false;
      bool contOrnament = false;
      bool reloadNote = false;
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          if (-1 != it->Param1)
          {
            track.SetEnvelopeType(it->Param1);
          }
          if (-1 != it->Param2)
          {
            EnvelopeTone = it->Param2;
            track.SetEnvelopeTone(EnvelopeTone);
          }
          break;
        case ENVELOPE_ON:
          dst.Envelope = true;
          break;
        case ENVELOPE_OFF:
          dst.Envelope = false;
          break;
        case NOISE:
          dst.BaseNoise = it->Param1;
          break;
        case CONT_SAMPLE:
          contSample = true;
          break;
        case CONT_ORNAMENT:
          contOrnament = true;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          dst.SlidingSteps = -1;  // infinite sliding
          break;
        case SLIDE:
        {
          dst.SlidingSteps = it->Param1;
          const int_t newSliding = (dst.Sliding | 0xf) ^ 0xf;
          dst.Glissade = -newSliding / (dst.SlidingSteps ? dst.SlidingSteps : 1);
          dst.Sliding = dst.Glissade * dst.SlidingSteps;
          break;
        }
        case SLIDE_NOTE:
        {
          dst.SlidingSteps = it->Param1;
          dst.SlidingTargetNote = it->Param2;
          const bool useToneSliding = it->Param3 != 0;
          const int_t absoluteSliding = track.GetSlidingDifference(dst.Note, dst.SlidingTargetNote);
          const int_t newSliding = absoluteSliding - (useToneSliding ? dst.Sliding / 16 : 0);
          dst.Glissade = 16 * newSliding / (dst.SlidingSteps ? dst.SlidingSteps : 1);
          reloadNote = true;
          break;
        }
        case AMPLITUDE_SLIDE:
          dst.VolSlideCounter = dst.VolSlideDelay = it->Param1;
          dst.VolSlideAddon = it->Param2;
          break;
        case BREAK_SAMPLE:
          dst.BreakSample = true;
          break;
        default:
          assert(!"Invalid cmd");
          break;
        }
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentNum = *ornament;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
        reloadNote = true;
      }
      if (reloadNote)
      {
        dst.CurrentNoise = dst.BaseNoise;
        if (dst.SlidingSteps <= 0)
        {
          dst.Sliding = 0;
        }
        if (!contSample)
        {
          dst.CurrentSampleNum = dst.SampleNum;
          dst.PosInSample = 0;
          dst.VolumeAddon = 0;
          dst.ToneDeviation = 0;
          dst.BreakSample = false;
        }
        if (!contOrnament)
        {
          dst.CurrentOrnamentNum = dst.OrnamentNum;
          dst.PosInOrnament = 0;
          dst.NoteAddon = 0;
        }
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
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
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const Sample& curSample = Data->Samples.Get(dst.CurrentSampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.CurrentOrnamentNum);
      const Ornament::Line& curOrnamentLine = curOrnament.GetLine(dst.PosInOrnament);

      // calculate volume addon
      if (dst.VolSlideCounter >= 2)
      {
        dst.VolSlideCounter--;
      }
      else if (dst.VolSlideCounter)
      {
        dst.VolumeAddon += dst.VolSlideAddon;
        dst.VolSlideCounter = dst.VolSlideDelay;
      }
      dst.VolumeAddon += curSampleLine.VolSlide;
      dst.VolumeAddon = Math::Clamp<int_t>(dst.VolumeAddon, -15, 15);

      // calculate tone
      dst.ToneDeviation += curSampleLine.ToneDeviation;
      dst.NoteAddon += curOrnamentLine.NoteAddon;
      const int_t halfTone = Math::Clamp<int8_t>(int8_t(dst.Note) + dst.NoteAddon, 0, 0x55);
      const int_t toneAddon = dst.ToneDeviation + dst.Sliding / 16;
      // apply tone
      channel.SetTone(halfTone, toneAddon);

      // apply level
      channel.SetLevel((dst.Volume + 1) * Math::Clamp<int_t>(dst.VolumeAddon + curSampleLine.Level, 0, 15) / 16);
      // apply envelope
      if (dst.Envelope && curSampleLine.EnableEnvelope)
      {
        channel.EnableEnvelope();
      }

      // calculate noise
      dst.CurrentNoise += curOrnamentLine.NoiseAddon;

      // mixer
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      if (curSampleLine.NoiseMask && curSampleLine.EnableEnvelope)
      {
        EnvelopeTone += curSampleLine.Adding;
        track.SetEnvelopeTone(EnvelopeTone);
      }
      else
      {
        dst.CurrentNoise += curSampleLine.Adding;
      }

      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise((dst.CurrentNoise + dst.Sliding / 256) & 0x1f);
      }
      else
      {
        channel.DisableNoise();
      }

      // recalc positions
      if (dst.SlidingSteps != 0)
      {
        if (dst.SlidingSteps > 0)
        {
          if (!--dst.SlidingSteps && LIMITER != dst.SlidingTargetNote)  // finish slide to note
          {
            dst.Note = dst.SlidingTargetNote;
            dst.SlidingTargetNote = LIMITER;
            dst.Sliding = dst.Glissade = 0;
          }
        }
        dst.Sliding += dst.Glissade;
      }

      if (dst.PosInSample++ >= curSample.GetLoopLimit())
      {
        if (!dst.BreakSample)
        {
          dst.PosInSample = curSample.GetLoop();
        }
        else if (dst.PosInSample >= curSample.GetSize())
        {
          dst.Enabled = false;
        }
      }
      if (dst.PosInOrnament++ >= curOrnament.GetLoopLimit())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }
    }

  private:
    const ModuleData::Ptr Data;
    uint_t EnvelopeTone = 0;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };

  class Factory : public AYM::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder)
      : Decoder(std::move(decoder))
    {}

    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      AYM::PropertiesHelper props(*properties);
      DataBuilder dataBuilder(props);
      if (const auto container = Decoder->Parse(rawData, dataBuilder))
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

  private:
    const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr Decoder;
  };

  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(std::move(decoder));
  }
}  // namespace Module::ASCSoundMaster
