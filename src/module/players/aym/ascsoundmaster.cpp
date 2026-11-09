/**
 *
 * @file
 *
 * @brief  ASCSoundMaster chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/ascsoundmaster.h"

#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_chiptune.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"

#include "binary/container.h"
#include "math/numeric.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::ASCSoundMaster
{
  // Supported commands and their parameters
  // Presentation: distinct notation commands get distinct values; don't pack variants.
  enum CmdType
  {
    // type, tone (-1 = unchanged)
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
    // glissade
    GLISS,
    // steps
    SLIDE,
    // steps,useToneSliding; target is the cell note
    SLIDE_NOTE,
    // period,delta
    AMPLITUDE_SLIDE,
    // no parameters
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
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetFrequencyTable(AYM::TABLE_ASM);
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
      auto& chan = Patterns.GetChannel();
      if (!chan.GetCommands().Find(BREAK_SAMPLE))
      {
        chan.SetEnabled(true);
      }
      chan.SetNote(note);
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
      auto& commands = Patterns.GetChannel().GetCommands();
      if (auto* cmd = commands.Find(ENVELOPE))
      {
        cmd->Param1 = int_t(type);
      }
      else
      {
        commands.Add(ENVELOPE, int_t(type), -1);
      }
    }

    void SetEnvelopeTone(uint_t tone) override
    {
      auto& commands = Patterns.GetChannel().GetCommands();
      if (auto* cmd = commands.Find(ENVELOPE))
      {
        cmd->Param2 = int_t(tone);
      }
      else
      {
        commands.Add(ENVELOPE, -1, int_t(tone));
      }
    }

    void SetEnvelope() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(ENVELOPE_ON);
    }

    void SetNoEnvelope() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(ENVELOPE_OFF);
    }

    void SetNoise(uint_t val) override
    {
      Patterns.GetChannel().GetCommands().AddOrUpdate(NOISE, val);
    }

    void SetContinueSample() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(CONT_SAMPLE);
    }

    void SetContinueOrnament() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(CONT_ORNAMENT);
    }

    void SetGlissade(int_t val) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(GLISS, val);
    }

    void SetSlide(int_t steps) override
    {
      Patterns.GetChannel().GetCommands().Add(SLIDE, steps);
    }

    void SetNoteSlide(int_t steps, bool useToneSliding) override
    {
      Patterns.GetChannel().GetCommands().Add(SLIDE_NOTE, steps, useToneSliding);
    }

    void SetVolumeSlide(uint_t period, int_t delta) override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(AMPLITUDE_SLIDE, period, delta);
    }

    void SetBreakSample() override
    {
      Patterns.GetChannel().GetCommands().AddOrReplace(BREAK_SAMPLE);
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

  class ToneSlider
  {
  public:
    void SetGlissade(int_t gliss)
    {
      Glissade = gliss;
      Steps = -1;  // infinite
    }

    void SetSlide(int_t steps, int_t range)
    {
      Steps = steps;
      Glissade = -range / (steps ? steps : 1);
      Sliding = steps ? -Glissade * steps : 0;
    }

    void Stop()
    {
      Steps = 0;
    }

    void ResetInfiniteSliding()
    {
      if (Steps <= 0)
      {
        Sliding = 0;
      }
    }

    int_t GetSliding() const
    {
      return Sliding;
    }

    void Update()
    {
      if (Steps != 0)
      {
        if (Steps > 0 && !--Steps)
        {
          Sliding = Glissade = 0;
        }
        Sliding += Glissade;
      }
    }

  private:
    int_t Sliding = 0;
    int_t Glissade = 0;
    int_t Steps = 0;  // negative means infinite
  };

  struct ChannelState
  {
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
    ToneSlider ToneSlide;

    void ResetBaseNoise()
    {
      BaseNoise = 0;
    }
  };

  class DataRenderer
  {
  public:
    explicit DataRenderer(const ModuleData& data)
      : Data(data)
    {}

    void Reset()
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    void SynthesizeData(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk)
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }

  private:
    void GetNewLineState(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Line)
      {
        for (auto& state : PlayerState)
        {
          state.ResetBaseNoise();
        }
      }
      if (const auto* const line = Data.GetLine(state))
      {
        line->ForEachChannel([&](auto chan, const auto& src) { GetNewChannelState(src, PlayerState[chan], track); });
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
      }
      dst.VolSlideCounter = 0;
      dst.ToneSlide.Stop();
      const uint_t* const note = src.GetNote();
      bool contSample = false;
      bool contOrnament = false;
      for (const auto& cmd : src.GetCommands())
      {
        switch (cmd.Type)
        {
        case ENVELOPE:
          if (-1 != cmd.Param1)
          {
            track.SetEnvelopeType(cmd.Param1);
          }
          if (-1 != cmd.Param2)
          {
            EnvelopeTone = cmd.Param2;
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
          dst.BaseNoise = cmd.Param1;
          break;
        case CONT_SAMPLE:
          contSample = true;
          break;
        case CONT_ORNAMENT:
          contOrnament = true;
          break;
        case GLISS:
          dst.ToneSlide.SetGlissade(cmd.Param1);
          break;
        case SLIDE:
          dst.ToneSlide.SetSlide(cmd.Param1, dst.ToneSlide.GetSliding() / 16 * 16);
          break;
        case SLIDE_NOTE:
        {
          const int_t currentSliding = dst.ToneSlide.GetSliding() / 16 * 16;
          const int_t toneSliding = cmd.Param2 ? currentSliding : 0;
          const int_t range = note ? -16 * track.GetSlidingDifference(dst.Note, *note) + toneSliding : currentSliding;
          dst.ToneSlide.SetSlide(cmd.Param1, range);
          break;
        }
        case AMPLITUDE_SLIDE:
          dst.VolSlideCounter = dst.VolSlideDelay = cmd.Param1;
          dst.VolSlideAddon = cmd.Param2;
          break;
        case BREAK_SAMPLE:
          dst.BreakSample = true;
          break;
        default:
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
      if (note)
      {
        dst.Note = *note;
        dst.CurrentNoise = dst.BaseNoise;
        dst.ToneSlide.ResetInfiniteSliding();
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

      const Sample& curSample = Data.Samples.Get(dst.CurrentSampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data.Ornaments.Get(dst.CurrentOrnamentNum);
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
      const int_t sliding = dst.ToneSlide.GetSliding();
      const int_t toneAddon = dst.ToneDeviation + sliding / 16;
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
        track.SetNoise((dst.CurrentNoise + sliding / 256) & 0x1f);
      }
      else
      {
        channel.DisableNoise();
      }

      // recalc positions
      dst.ToneSlide.Update();

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
    const ModuleData& Data;
    uint_t EnvelopeTone = 0;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };
}  // namespace Module::ASCSoundMaster

namespace Module::AYM
{
  AYM::Chiptune::Ptr CreateModule(const Binary::Container& rawData, PropertiesHelper& props,
                                  Formats::Chiptune::ASCSoundMaster::Parser parse)
  {
    ASCSoundMaster::DataBuilder dataBuilder(props);
    if (const auto container = parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<ASCSoundMaster::DataRenderer>(dataBuilder.CaptureResult());
    }
    return {};
  }

  Chiptune::Ptr CreateASCSoundMasterChiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    return CreateModule(rawData, props, Formats::Chiptune::ASCSoundMaster::Ver1::Parse);
  }

  Chiptune::Ptr CreateASCSoundMaster0Chiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    return CreateModule(rawData, props, Formats::Chiptune::ASCSoundMaster::Ver0::Parse);
  }
}  // namespace Module::AYM
