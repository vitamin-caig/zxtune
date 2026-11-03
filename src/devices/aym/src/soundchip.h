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

#include "devices/aym/src/renderers.h"
#include "devices/aym/src/volume_table.h"
#include "devices/details/chunks_cache.h"

#include "parameters/tracking_helper.h"

#include <utility>

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

    void RenderData(typename Traits::DataChunkType src) override
    {
      Chunks.Add(std::move(src));
    }

    void Reset() override
    {
      Params.Reset();
      PSG.Reset();
      Renderers.Reset();
      Chunks = {};
      SynchronizeParameters();
    }

    Sound::Chunk RenderTill(Stamp till) override
    {
      SynchronizeParameters();
      Sound::Chunk result;
      for (const auto& event : Chunks.ConsumeBefore(till))
      {
        Renderers.Render(event.TimeStamp, &result);
        PSG.SetNewData(event.Data);
      }
      Renderers.Render(till, &result);
      return result;
    }

  private:
    void SynchronizeParameters()
    {
      const uint_t AYM_CLOCK_DIVISOR = 8;

      if (Params.IsChanged())
      {
        PSG.SetMuteMask(Params->MuteMask());
        PSG.SetDutyCycle(Params->DutyCycleValue(), Params->DutyCycleMask());
        const uint_t clock = Params->ClockFreq() / AYM_CLOCK_DIVISOR;
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
    Details::ChunksCache<typename Traits::DataChunkType, Stamp> Chunks;
  };
}  // namespace Devices::AYM
