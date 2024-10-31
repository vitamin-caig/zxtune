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

#include "renderers.h"
#include "volume_table.h"

#include <parameters/tracking_helper.h>

namespace Devices::AYM
{
  template<class Traits>
  class SoundChip : public Traits::ChipBaseType
  {
  public:
    SoundChip(ChipParameters::Ptr params, MixerType::Ptr mixer)
      : Params(std::move(params))
      , Mixer(std::move(mixer))
      , PSG(VolTable)
      , Renderers(Clock, PSG)
    {
      SoundChip::Reset();
    }

    void RenderData(const typename Traits::DataChunkType& src) override
    {
      if (Clock.HasSamplesBefore(src.TimeStamp))
      {
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
        RenderedData.reserve(RenderedData.size() + Clock.SamplesTill(end));
        for (const auto& chunk : src)
        {
          Renderers.Render(chunk.TimeStamp, &RenderedData);
          PSG.SetNewData(chunk.Data);
        }
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
      SynchronizeParameters();
    }

    Sound::Chunk RenderTill(Stamp stamp) override
    {
      Sound::Chunk result;
      if (RenderedData.empty())
      {
        result = Renderers.Render(stamp, Clock.SamplesTill(stamp));
      }
      else
      {
        Renderers.Render(stamp, &RenderedData);
        result.swap(RenderedData);
        RenderedData.reserve(result.size());
      }
      SynchronizeParameters();
      return result;
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
        VolTable.SetParameters(Params->Type(), Params->Layout(), *Mixer);
      }
    }

  private:
    Parameters::TrackingHelper<ChipParameters> Params;
    const MixerType::Ptr Mixer;
    MultiVolumeTable VolTable;
    typename Traits::PSGType PSG;
    ClockSource Clock;
    RenderersSet<typename Traits::PSGType> Renderers;
    Sound::Chunk RenderedData;
  };
}  // namespace Devices::AYM
