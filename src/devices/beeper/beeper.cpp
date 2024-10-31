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

    void RenderData(const std::vector<DataChunk>& src) override
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
          Renderer.Render(chunk.TimeStamp, &RenderedData);
          PSG.SetNewData(chunk.Level);
        }
      }
      else
      {
        for (const auto& chunk : src)
        {
          PSG.SetNewData(chunk.Level);
        }
      }
    }

    void Reset() override
    {
      Params.Reset();
      PSG.Reset();
      Clock.Reset();
      ClockFreq = 0;
      SoundFreq = 0;
      SynchronizeParameters();
    }

    Sound::Chunk RenderTill(Stamp stamp) override
    {
      Sound::Chunk result;
      if (RenderedData.empty())
      {
        result = Renderer.Render(stamp, Clock.SamplesTill(stamp));
      }
      else
      {
        Renderer.Render(stamp, &RenderedData);
        result.swap(RenderedData);
        RenderedData.reserve(result.size());
      }
      SynchronizeParameters();
      return result;
    }

  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        const uint64_t clkFreq = Params->ClockFreq();
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
    uint64_t ClockFreq = 0;
    uint_t SoundFreq = 0;
    Sound::Chunk RenderedData;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params)
  {
    return MakePtr<ChipImpl>(std::move(params));
  }
}  // namespace Devices::Beeper
