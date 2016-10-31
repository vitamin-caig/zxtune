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
      , Mixer(std::move(mixer))
      , Target(std::move(target))
      , PSG(VolTable)
      , Clock()
      , Renderers(Clock, PSG)
    {
      SoundChip::Reset();
    }

    void RenderData(const typename Traits::DataChunkType& src) override
    {
      if (Clock.HasSamplesBefore(src.TimeStamp))
      {
        SynchronizeParameters();
        RenderTill(src.TimeStamp);
      }
      PSG.SetNewData(src.Data);
    }

    void RenderData(const std::vector<typename Traits::DataChunkType>& src) override
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
        for (const auto& chunk : src)
        {
          Renderers.Render(chunk.TimeStamp, builder);
          PSG.SetNewData(chunk.Data);
        }
        Target->ApplyData(builder.CaptureResult());
        Target->Flush();
      }
      else
      {
        for (const auto& chunk : src)
        {
          PSG.SetNewData(chunk.Data);
        }
      }
    }

    void Reset() override
    {
      Params.Reset();
      PSG.Reset();
      Renderers.Reset();
    }

    void GetState(MultiChannelState& state) const override
    {
      MultiChannelState res;
      res.reserve(Traits::VOICES);
      PSG.GetState(res);
      for (auto & re : res)
      {
        re.Band = Analyser.GetBandByPeriod(re.Band);
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
      Target->ApplyData(builder.CaptureResult());
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
