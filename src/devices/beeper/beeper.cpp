/**
 *
 * @file
 *
 * @brief  Beeper support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "devices/beeper.h"

#include "devices/details/chunks_cache.h"
#include "devices/details/renderers.h"

#include "parameters/tracking_helper.h"

#include "make_ptr.h"

#include <utility>

namespace Devices::Beeper
{
  class BeeperPSG
  {
  public:
    void SetNewData(bool level)
    {
      Levels = level ? Sound::Sample(Sound::Sample::MAX / 4, Sound::Sample::MAX / 4) : Sound::Sample();
    }

    void Tick(uint_t /*ticks*/) {}

    Sound::Sample GetLevels() const
    {
      return Levels;
    }

    void Reset()
    {
      Levels = Sound::Sample();
    }

  private:
    Sound::Sample Levels;
  };

  class ChipImpl : public Chip
  {
  public:
    explicit ChipImpl(ChipParameters::Ptr params)
      : Params(std::move(params))
      , Renderer(Clock, PSG)
    {
      SynchronizeParameters();
    }

    void RenderData(DataChunk src) override
    {
      Chunks.Add(std::move(src));
    }

    void Reset() override
    {
      Params.Reset();
      PSG.Reset();
      Clock.Reset();
      ClockFreq = 0;
      SoundFreq = 0;
      Chunks = {};
      SynchronizeParameters();
    }

    Sound::Chunk RenderTill(Stamp till) override
    {
      SynchronizeParameters();
      Sound::Chunk result;
      for (const auto& event : Chunks.ConsumeBefore(till))
      {
        Renderer.Render(event.TimeStamp, &result);
        PSG.SetNewData(event.Level);
      }
      Renderer.Render(till, &result);
      return result;
    }

  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        const uint_t clkFreq = Params->ClockFreq();
        const uint_t sndFreq = Params->SoundFreq();
        if (clkFreq != ClockFreq || sndFreq != SoundFreq)
        {
          Clock.SetFrequency(clkFreq, sndFreq);
          Renderer.SetClockFrequency(clkFreq);
        }
      }
    }

  private:
    Parameters::TrackingHelper<ChipParameters> Params;
    BeeperPSG PSG;
    Details::ClockSource<Stamp> Clock;
    Details::HQRenderer<Stamp, BeeperPSG> Renderer;
    uint_t ClockFreq = 0;
    uint_t SoundFreq = 0;
    Details::ChunksCache<DataChunk, Stamp> Chunks;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params)
  {
    return MakePtr<ChipImpl>(std::move(params));
  }
}  // namespace Devices::Beeper
