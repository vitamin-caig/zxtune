/**
* 
* @file
*
* @brief  Beeper support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include <devices/beeper.h>
#include <devices/details/parameters_helper.h>
#include <devices/details/renderers.h>

namespace Devices
{
namespace Beeper
{
  class BeeperPSG
  {
  public:
    void SetNewData(bool level)
    {
      Levels = level ? Sound::Sample(Sound::Sample::MAX / 2, Sound::Sample::MAX / 2) : Sound::Sample();
    }
    
    void Tick(uint_t /*ticks*/)
    {
    }

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
    ChipImpl(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
      : Params(params)
      , Target(target)
      , Renderer(Clock, PSG)
      , ClockFreq()
      , SoundFreq()
    {
    }
    
    virtual void RenderData(const std::vector<DataChunk>& src)
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
        for (std::vector<DataChunk>::const_iterator it = src.begin(), lim = src.end(); it != lim; ++it)
        {
          const DataChunk& chunk = *it;
          Renderer.Render(chunk.TimeStamp, builder);
          PSG.SetNewData(it->Level);
        }
        Target->ApplyData(builder.GetResult());
        Target->Flush();
      }
      else
      {
        for (std::vector<DataChunk>::const_iterator it = src.begin(), lim = src.end(); it != lim; ++it)
        {
          PSG.SetNewData(it->Level);
        }
      }
    }

    virtual void Reset()
    {
      Params.Reset();
      PSG.Reset();
      Clock.Reset();
      ClockFreq = 0;
      SoundFreq = 0;
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
    Details::ParametersHelper<ChipParameters> Params;
    const Sound::Receiver::Ptr Target;
    BeeperPSG PSG;
    Details::ClockSource<Stamp> Clock;
    Details::HQRenderer<Stamp, BeeperPSG> Renderer;
    uint64_t ClockFreq;
    uint_t SoundFreq;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
  {
    return boost::make_shared<ChipImpl>(params, target);
  }
}
}
