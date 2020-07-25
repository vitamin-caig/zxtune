/**
* 
* @file
*
* @brief  FM chips base implementation
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "Ym2203_Emu.h"
//library includes
#include <devices/fm.h>
#include <devices/details/analysis_map.h>
#include <math/numeric.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <time/duration.h>

namespace Devices
{
namespace FM
{
  namespace Details
  {
    typedef int32_t YM2203SampleType;
    typedef std::shared_ptr<void> ChipPtr;

    static_assert(sizeof(YM2203SampleType) == sizeof(Sound::Sample), "Incompatible sample types");

    class ClockSource
    {
      typedef Math::FixedPoint<Stamp::ValueType, Stamp::PER_SECOND> FixedPoint;
    public:
      void SetFrequency(uint_t sndFreq)
      {
        Frequency = FixedPoint(sndFreq, Stamp::PER_SECOND);
      }

      void Reset()
      {
        LastTime = Stamp();
        Frequency = 0;
      }

      // @return samples to render
      uint_t AdvanceTo(Stamp stamp)
      {
        if (stamp.Get() <= LastTime.Get())
        {
          return 0;
        }
        const uint64_t delta = stamp.Get() - LastTime.Get();
        const uint64_t res = (Frequency * delta).Integer();
        LastTime += Time::Duration<TimeUnit>((FixedPoint(res) / Frequency).Integer());
        return res;
      }
    private:
      Stamp LastTime;
      FixedPoint Frequency;
    };

    class ChipAdapterHelper
    {
    public:
      ChipAdapterHelper()
        : LastClockrate()
        , LastSoundFreq()
      {
      }

      bool SetNewParams(uint64_t clock, uint_t sndFreq)
      {
        Analyser.SetClockRate(clock);
        if (clock != LastClockrate || sndFreq != LastSoundFreq)
        {
          LastClockrate = clock;
          LastSoundFreq = sndFreq;
          return true;
        }
        return false;
      }

      ChipPtr CreateChip() const
      {
        return ChipPtr(::YM2203Init(LastClockrate, LastSoundFreq), &::YM2203Shutdown);
      }

      static void ConvertSamples(const YM2203SampleType* inBegin, const YM2203SampleType* inEnd, Sound::Sample* out)
      {
        std::transform(inBegin, inEnd, out, &ConvertToSample);
      }

      void ConvertState(const uint_t* attenuations, const uint_t* periods, DeviceState& res) const
      {
        for (uint_t idx = 0; idx != VOICES; ++idx)
        {
          const uint_t MAX_ATTENUATION = 1024;
          if (attenuations[idx] < MAX_ATTENUATION)
          {
            const uint_t band = Analyser.GetBandByPeriod(periods[idx]);
            const LevelType level(MAX_ATTENUATION - attenuations[idx], MAX_ATTENUATION);
            res.Set(band, level);
          }
        }
      }
    private:
      static Sound::Sample ConvertToSample(YM2203SampleType level)
      {
        using namespace Sound;
        const Sample::WideType val = Math::Clamp<Sample::WideType>(level + Sample::MID, Sample::MIN, Sample::MAX);
        return Sound::Sample(val, val);
      }
    private:
      uint64_t LastClockrate;
      uint_t LastSoundFreq;
      Devices::Details::AnalysisMap Analyser;
    };

    template<class ChipTraits>
    class BaseChip : public ChipTraits::BaseClass
    {
    public:
      BaseChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
        : Params(std::move(params))
        , Target(std::move(target))
      {
        BaseChip::Reset();
      }

      void RenderData(const typename ChipTraits::DataChunkType& src) override
      {
        if (const uint_t samples = Clock.AdvanceTo(src.TimeStamp))
        {
          SynchronizeParameters();
          Render(samples);
        }
        Adapter.WriteRegisters(src.Data);
      }

      void Reset() override
      {
        Params.Reset();
        Adapter.Reset();
        Clock.Reset();
        SynchronizeParameters();
      }

      DeviceState GetState() const override
      {
        return Adapter.GetState();
      }
    private:
      void SynchronizeParameters()
      {
        if (Params.IsChanged())
        {
          const uint64_t clockFreq = Params->ClockFreq();
          const uint_t sndFreq = Params->SoundFreq();
          Adapter.SetParams(clockFreq, sndFreq);
          Clock.SetFrequency(sndFreq);
        }
      }

      void Render(uint_t samples)
      {
        Sound::ChunkBuilder builder;
        builder.Reserve(samples);
        Adapter.RenderSamples(samples, builder);
        Target->ApplyData(builder.CaptureResult());
        Target->Flush();
      }
    private:
      Parameters::TrackingHelper<ChipParameters> Params;
      const Sound::Receiver::Ptr Target;
      typename ChipTraits::AdapterType Adapter;
      ClockSource Clock;
    };
  }
}
}
