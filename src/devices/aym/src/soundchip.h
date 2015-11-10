/**
* 
* @file
*
* @brief  AY/YM-based chips base implementation
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "renderers.h"
#include "volume_table.h"
//library includes
#include <devices/details/analysis_map.h>
#include <parameters/tracking_helper.h>

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
      if (Clock.HasSamplesBefore(src.TimeStamp))
      {
        SynchronizeParameters();
        RenderTill(src.TimeStamp);
      }
      PSG.SetNewData(src.Data);
    }

    virtual void RenderData(const std::vector<typename Traits::DataChunkType>& src)
    {
      if (src.empty())
      {
        return;
      }
      const Stamp end = src.back().TimeStamp;
      if (Clock.HasSamplesBefore(end))
      {
        SynchronizeParameters();
        const uint_t samples = Clock.SamplesTill(end);
        Sound::ChunkBuilder builder;
        builder.Reserve(samples);
        for (typename std::vector<typename Traits::DataChunkType>::const_iterator it = src.begin(), lim = src.end(); it != lim; ++it)
        {
          const typename Traits::DataChunkType& chunk = *it;
          Renderers.Render(chunk.TimeStamp, builder);
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

    void RenderTill(Stamp stamp)
    {
      const uint_t samples = Clock.SamplesTill(stamp);
      Sound::ChunkBuilder builder;
      builder.Reserve(samples);
      Renderers.Render(stamp, samples, builder);
      Target->ApplyData(builder.GetResult());
      Target->Flush();
    }
  private:
    Parameters::TrackingHelper<ChipParameters> Params;
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
