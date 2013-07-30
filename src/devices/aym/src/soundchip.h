/*
Abstract:
  AY/YM chips interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_AYM_SOUNDCHIP_H_DEFINED
#define DEVICES_AYM_SOUNDCHIP_H_DEFINED

//local includes
#include "renderers.h"
#include "volume_table.h"
//library includes
#include <devices/details/analysis_map.h>
#include <devices/details/parameters_helper.h>

namespace Devices
{
namespace AYM
{
  template<class Traits>
  class SoundChip : public Traits::ChipBaseType
  {
  public:
    SoundChip(ChipParameters::Ptr params, MixerType::Ptr mixer, Sound::Receiver::Ptr target)
      : Params(params)
      , Mixer(mixer)
      , Target(target)
      , PSG(VolTable)
      , Clock()
      , Renderers(Clock, PSG)
    {
      SoundChip::Reset();
    }

    virtual void RenderData(const typename Traits::DataChunkType& src)
    {
      if (Clock.GetNextSampleTime() < src.TimeStamp)
      {
        SynchronizeParameters();
        RenderChunksTill(src.TimeStamp);
      }
      PSG.SetNewData(src.Data);
    }

    virtual void RenderData(const std::vector<typename Traits::DataChunkType>& src)
    {
      if (src.empty())
      {
        return;
      }
      const Stamp till = src.back().TimeStamp;
      if (Clock.GetNextSampleTime() < till)
      {
        SynchronizeParameters();
        Sound::ChunkBuilder builder;
        builder.Reserve(Clock.SamplesTill(till));
        for (typename std::vector<typename Traits::DataChunkType>::const_iterator it = src.begin(), lim = src.end(); it != lim; ++it)
        {
          const typename Traits::DataChunkType& chunk = *it;
          if (Clock.GetCurrentTime() < chunk.TimeStamp)
          {
            Renderers.Render(chunk.TimeStamp, builder);
          }
          PSG.SetNewData(it->Data);
        }
        Target->ApplyData(builder.GetResult());
        Target->Flush();
      }
      else
      {
        for (typename std::vector<typename Traits::DataChunkType>::const_iterator it = src.begin(), lim = src.end(); it != lim; ++it)
        {
          PSG.SetNewData(it->Data);
        }
      }
    }

    virtual void Reset()
    {
      Params.Reset();
      PSG.Reset();
      Renderers.Reset();
    }

    virtual void GetState(MultiChannelState& state) const
    {
      MultiChannelState res;
      res.reserve(Traits::VOICES);
      PSG.GetState(res);
      for (MultiChannelState::iterator it = res.begin(), lim = res.end(); it != lim; ++it)
      {
        it->Band = Analyser.GetBandByPeriod(it->Band);
      }
      state.swap(res);
    }
  private:
    void SynchronizeParameters()
    {
      const uint_t AYM_CLOCK_DIVISOR = 8;

      if (Params.IsChanged())
      {
        PSG.SetDutyCycle(Params->DutyCycleValue(), Params->DutyCycleMask());
        const uint64_t clock = Params->ClockFreq() / AYM_CLOCK_DIVISOR;
        const uint_t sndFreq = Params->SoundFreq();
        Renderers.SetFrequency(clock, sndFreq);
        Renderers.SetInterpolation(Params->Interpolation());
        Analyser.SetClockRate(clock);
        VolTable.SetParameters(Params->Type(), Params->Layout(), *Mixer);
      }
    }

    void RenderChunksTill(Stamp stamp)
    {
      const uint_t samples = Clock.SamplesTill(stamp);
      Sound::ChunkBuilder builder;
      builder.Reserve(samples);
      Renderers.Render(stamp, builder);
      Target->ApplyData(builder.GetResult());
      Target->Flush();
    }
  private:
    Details::ParametersHelper<ChipParameters> Params;
    const MixerType::Ptr Mixer;
    const Sound::Receiver::Ptr Target;
    MultiVolumeTable VolTable;
    typename Traits::PSGType PSG;
    ClockSource Clock;
    Details::AnalysisMap Analyser;
    RenderersSet<typename Traits::PSGType> Renderers;
  };
}
}

#endif
