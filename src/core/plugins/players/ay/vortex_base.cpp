/*
Abstract:
  Vortex-based modules playback support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ts_base.h"
#include "vortex_base.h"
//common includes
#include <error_tools.h>
#include <tools.h>
//boost includes
#include <boost/make_shared.hpp>

#define FILE_TAG 023C2245

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const uint_t LIMITER = ~uint_t(0);

  typedef boost::array<uint8_t, 256> VolumeTable;

  //Volume table of Pro Tracker 3.3x - 3.4x
  const VolumeTable Vol33_34 =
  { {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x09, 0x0a, 0x0b,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  } };

  //Volume table of Pro Tracker 3.5x
  const VolumeTable Vol35 =
  { {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09,
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0a,
    0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
    0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c,
    0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  } };

  //helper for sliding processing
  struct Slider
  {
    Slider() : Period(), Value(), Counter(), Delta()
    {
    }
    uint_t Period;
    int_t Value;
    uint_t Counter;
    int_t Delta;

    bool Update()
    {
      if (Counter && !--Counter)
      {
        Value += Delta;
        Counter = Period;
        return true;
      }
      return false;
    }

    void Reset()
    {
      Counter = 0;
      Value = 0;
    }
  };

  //internal per-channel state type
  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false)
      , Note()
      , SampleNum(0), PosInSample(0)
      , OrnamentNum(0), PosInOrnament(0)
      , Volume(15), VolSlide(0)
      , ToneSlider(), SlidingTargetNote(LIMITER), ToneAccumulator(0)
      , EnvSliding(), NoiseSliding()
      , VibrateCounter(0), VibrateOn(), VibrateOff()
    {
    }

    bool Enabled;
    bool Envelope;
    uint_t Note;
    uint_t SampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t PosInOrnament;
    uint_t Volume;
    int_t VolSlide;
    Slider ToneSlider;
    uint_t SlidingTargetNote;
    int_t ToneAccumulator;
    int_t EnvSliding;
    int_t NoiseSliding;
    uint_t VibrateCounter;
    uint_t VibrateOn;
    uint_t VibrateOff;
  };

  //internal common state type
  struct CommonState
  {
    CommonState()
      : EnvBase()
      , NoiseBase()
    {
    }

    uint_t EnvBase;
    Slider EnvSlider;
    uint_t NoiseBase;
  };

  struct VortexState
  {
    boost::array<ChannelState, AYM::CHANNELS> ChanState;
    CommonState CommState;
  };

  //simple player type
  class VortexDataRenderer : public AYMDataRenderer
  {
  public:
    VortexDataRenderer(Vortex::Track::ModuleData::Ptr data, uint_t version)
      : Data(data)
      , Version(version)
      , VolTable(version <= 4 ? Vol33_34 : Vol35)
    {
    }

    virtual void Reset()
    {
      PlayerState = VortexState();
    }

    virtual void SynthesizeData(const TrackState& state, AYMTrackSynthesizer& synthesizer)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, synthesizer);
      }
      SynthesizeChannelsData(synthesizer);
    }
  private:
    void GetNewLineState(const TrackState& state, AYMTrackSynthesizer& synthesizer)
    {
      if (0 == state.Line())
      {
        PlayerState.CommState.NoiseBase = 0;
      }

      if (const Vortex::Track::Line* line = Data->Patterns[state.Pattern()].GetLine(state.Line()))
      {
        for (uint_t chan = 0; chan != line->Channels.size(); ++chan)
        {
          const Vortex::Track::Line::Chan& src = line->Channels[chan];
          if (src.Empty())
          {
            continue;
          }
          GetNewChannelState(src, PlayerState.ChanState[chan], synthesizer);
        }
      }
    }

    void GetNewChannelState(const Vortex::Track::Line::Chan& src, ChannelState& dst, AYMTrackSynthesizer& synthesizer)
    {
      if (src.Enabled)
      {
        dst.PosInSample = dst.PosInOrnament = 0;
        dst.VolSlide = dst.EnvSliding = dst.NoiseSliding = 0;
        dst.ToneSlider.Reset();
        dst.ToneAccumulator = 0;
        dst.VibrateCounter = 0;
        dst.Enabled = *src.Enabled;
      }
      if (src.Note)
      {
        dst.Note = *src.Note;
      }
      if (src.SampleNum)
      {
        dst.SampleNum = *src.SampleNum;
      }
      if (src.OrnamentNum)
      {
        dst.OrnamentNum = *src.OrnamentNum;
        dst.PosInOrnament = 0;
      }
      if (src.Volume)
      {
        dst.Volume = *src.Volume;
      }
      for (Vortex::Track::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
      {
        switch (it->Type)
        {
        case Vortex::GLISS:
          dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
          dst.ToneSlider.Delta = it->Param2;
          dst.SlidingTargetNote = LIMITER;
          dst.VibrateCounter = 0;
          if (0 == dst.ToneSlider.Counter && Version >= 7)
          {
            ++dst.ToneSlider.Counter;
          }
          break;
        case Vortex::GLISS_NOTE:
          dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
          dst.ToneSlider.Delta = it->Param2;
          dst.SlidingTargetNote = it->Param3;
          dst.VibrateCounter = 0;
          //tone up                                     freq up
          if (bool(dst.Note < dst.SlidingTargetNote) != bool(dst.ToneSlider.Delta < 0))
          {
            dst.ToneSlider.Delta = -dst.ToneSlider.Delta;
          }
          break;
        case Vortex::SAMPLEOFFSET:
          dst.PosInSample = it->Param1;
          break;
        case Vortex::ORNAMENTOFFSET:
          dst.PosInOrnament = it->Param1;
          break;
        case Vortex::VIBRATE:
          dst.VibrateCounter = dst.VibrateOn = it->Param1;
          dst.VibrateOff = it->Param2;
          dst.ToneSlider.Value = 0;
          dst.ToneSlider.Counter = 0;
          break;
        case Vortex::SLIDEENV:
          PlayerState.CommState.EnvSlider.Period = PlayerState.CommState.EnvSlider.Counter = it->Param1;
          PlayerState.CommState.EnvSlider.Delta = it->Param2;
          break;
        case Vortex::ENVELOPE:
          synthesizer.SetEnvelopeType(it->Param1);
          PlayerState.CommState.EnvBase = it->Param2;
          dst.Envelope = true;
          PlayerState.CommState.EnvSlider.Reset();
          dst.PosInOrnament = 0;
          break;
        case Vortex::NOENVELOPE:
          dst.Envelope = false;
          dst.PosInOrnament = 0;
          break;
        case Vortex::NOISEBASE:
          PlayerState.CommState.NoiseBase = it->Param1;
          break;
        case Vortex::TEMPO:
          //ignore
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(AYMTrackSynthesizer& synthesizer)
    {
      int_t envelopeAddon = 0;
      for (uint_t chan = 0; chan != PlayerState.ChanState.size(); ++chan)
      {
        const AYMChannelSynthesizer& chanSynt = synthesizer.GetChannel(chan);
        ChannelState& dst = PlayerState.ChanState[chan];
        SynthesizeChannel(dst, chanSynt, synthesizer, envelopeAddon);
        //update vibrato
        if (dst.VibrateCounter > 0 && !--dst.VibrateCounter)
        {
          dst.Enabled = !dst.Enabled;
          dst.VibrateCounter = dst.Enabled ? dst.VibrateOn : dst.VibrateOff;
        }
      }
      const int_t envPeriod = envelopeAddon + PlayerState.CommState.EnvSlider.Value + PlayerState.CommState.EnvBase;
      synthesizer.SetEnvelopeTone(envPeriod);
      PlayerState.CommState.EnvSlider.Update();
    }

    void SynthesizeChannel(ChannelState& dst, const AYMChannelSynthesizer& chanSynth, AYMTrackSynthesizer& trackSynth, int_t& envelopeAddon)
    {
      if (!dst.Enabled)
      {
        chanSynth.SetLevel(0);
        return;
      }

      const Vortex::Track::Sample& curSample = Data->Samples[dst.SampleNum];
      const Vortex::Track::Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Vortex::Track::Ornament& curOrnament = Data->Ornaments[dst.OrnamentNum];

      //calculate tone
      const int_t toneAddon = curSampleLine.ToneOffset + dst.ToneAccumulator;
      if (curSampleLine.KeepToneOffset)
      {
        dst.ToneAccumulator = toneAddon;
      }
      const int_t halfTones = int_t(dst.Note) + curOrnament.GetLine(dst.PosInOrnament);
      const int_t toneOffset = dst.ToneSlider.Value + toneAddon;

      //apply tone
      chanSynth.SetTone(halfTones, toneOffset);
      if (curSampleLine.ToneMask)
      {
        chanSynth.DisableTone();
      }
      //apply level
      dst.VolSlide = clamp<int_t>(dst.VolSlide + curSampleLine.VolumeSlideAddon, -15, 15);
      chanSynth.SetLevel(GetVolume(dst.Volume, clamp<int_t>(dst.VolSlide + curSampleLine.Level, 0, 15)));
      //apply envelope
      if (dst.Envelope && !curSampleLine.EnvMask)
      {
        chanSynth.EnableEnvelope();
      }
      //apply noise
      if (curSampleLine.NoiseMask)
      {
        const int_t envAddon = curSampleLine.NoiseOrEnvelopeOffset + dst.EnvSliding;
        if (curSampleLine.KeepNoiseOrEnvelopeOffset)
        {
          dst.EnvSliding = envAddon;
        }
        envelopeAddon += envAddon;
        chanSynth.DisableNoise();
      }
      else
      {
        const int_t noiseAddon = curSampleLine.NoiseOrEnvelopeOffset + dst.NoiseSliding;
        if (curSampleLine.KeepNoiseOrEnvelopeOffset)
        {
          dst.NoiseSliding = noiseAddon;
        }
        trackSynth.SetNoise(PlayerState.CommState.NoiseBase + noiseAddon);
      }

      if (dst.ToneSlider.Update() &&
          LIMITER != dst.SlidingTargetNote)
      {
        const int_t absoluteSlidingRange = trackSynth.GetSlidingDifference(halfTones, dst.SlidingTargetNote);
        const int_t realSlidingRange = absoluteSlidingRange - toneOffset;

        if ((dst.ToneSlider.Delta > 0 && realSlidingRange < dst.ToneSlider.Delta) ||
          (dst.ToneSlider.Delta < 0 && realSlidingRange > dst.ToneSlider.Delta))
        {
          //slided to target note
          dst.Note = dst.SlidingTargetNote;
          dst.SlidingTargetNote = LIMITER;
          dst.ToneSlider.Value = 0;
          dst.ToneSlider.Counter = 0;
        }
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        dst.PosInSample = curSample.GetLoop();
      }
      if (++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }
    }

    uint_t GetVolume(uint_t volume, uint_t level)
    {
      return VolTable[volume * 16 + level];
    }
  private:
    const Vortex::Track::ModuleData::Ptr Data;
    const uint_t Version;
    const VolumeTable& VolTable;
    VortexState PlayerState;
  };
}

namespace ZXTune
{
  namespace Module
  {
    namespace Vortex
    {
      String GetFreqTable(NoteTable table, uint_t version)
      {
        switch (table)
        {
        case Vortex::PROTRACKER:
          return version <= 3 ? TABLE_PROTRACKER3_3 : TABLE_PROTRACKER3_4;
        case Vortex::SOUNDTRACKER:
          return TABLE_SOUNDTRACKER;
        case Vortex::ASM:
          return version <= 3 ? TABLE_PROTRACKER3_3_ASM : TABLE_PROTRACKER3_4_ASM;
        case Vortex::REAL:
          return version <= 3 ? TABLE_PROTRACKER3_3_REAL : TABLE_PROTRACKER3_4_REAL;
        case Vortex::NATURAL:
          return TABLE_NATURAL_SCALED;
        default:
          //assert(!"Unknown frequency table for Vortex-based modules");
          return TABLE_PROTRACKER3_3;
        }
      }


      Player::Ptr CreatePlayer(Information::Ptr info, Track::ModuleData::Ptr data,
         uint_t version, const String& freqTableName, AYM::Chip::Ptr device)
      {
        const AYMDataRenderer::Ptr renderer = boost::make_shared<VortexDataRenderer>(data, version);
        return CreateAYMTrackPlayer(info, data, renderer, device, freqTableName);
      }
    }
  }
}
