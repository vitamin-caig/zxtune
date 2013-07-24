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
#include <devices/details/chunks_cache.h>
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
      BufferedData.Add(src);
    }

    virtual void Flush()
    {
      const Stamp till = BufferedData.GetTillTime();
      if (!(till == Stamp(0)))
      {
        SynchronizeParameters();
        Sound::ChunkBuilder builder;
        builder.Reserve(Clock.SamplesTill(till));
        RenderChunks(builder);
        Target->ApplyData(builder.GetResult());
      }
      Target->Flush();
    }

    virtual void Reset()
    {
      Params.Reset();
      PSG.Reset();
      BufferedData.Reset();
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

    void RenderChunks(Sound::ChunkBuilder& builder)
    {
      Renderer& source = Renderers.Get();
      for (const typename Traits::DataChunkType* it = BufferedData.GetBegin(), *lim = BufferedData.GetEnd(); it != lim; ++it)
      {
        const typename Traits::DataChunkType& chunk = *it;
        if (Clock.GetCurrentTime() < chunk.TimeStamp)
        {
          source.Render(chunk.TimeStamp, builder);
        }
        PSG.SetNewData(chunk.Data);
      }
      BufferedData.Reset();
    }
  private:
    Details::ParametersHelper<ChipParameters> Params;
    const MixerType::Ptr Mixer;
    const Sound::Receiver::Ptr Target;
    MultiVolumeTable VolTable;
    typename Traits::PSGType PSG;
    ClockSource Clock;
    Details::ChunksCache<typename Traits::DataChunkType, Stamp> BufferedData;
    Details::AnalysisMap Analyser;
    RenderersSet<typename Traits::PSGType> Renderers;
  };
}
}

#endif
