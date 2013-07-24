/*
Abstract:
  FM chips base implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "Ym2203_Emu.h"
//common includes
#include <tools.h>
//library includes
#include <devices/fm.h>
#include <devices/details/analysis_map.h>
#include <devices/details/chunks_cache.h>
#include <devices/details/clock_source.h>
#include <devices/details/parameters_helper.h>
#include <math/numeric.h>
#include <sound/chunk_builder.h>
#include <time/oscillator.h>
//boost includes
#include <boost/bind.hpp>

namespace Devices
{
namespace FM
{
  namespace Details
  {
    typedef int32_t YM2203SampleType;
    typedef boost::shared_ptr<void> ChipPtr;

    BOOST_STATIC_ASSERT(sizeof(YM2203SampleType) == sizeof(Sound::Sample));

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

      template<class It>
      static void WriteRegisters(It begin, It end, void* chip)
      {
        for (It it = begin; it != end; ++it)
        {
          ::YM2203WriteRegs(chip, it->Index, it->Value);
        }
      }

      static void ConvertSamples(const YM2203SampleType* inBegin, const YM2203SampleType* inEnd, Sound::Sample* out)
      {
        std::transform(inBegin, inEnd, out, &ConvertToSample);
      }

      void ConvertState(const uint_t* attenuations, const uint_t* periods, MultiChannelState& res) const
      {
        for (uint_t idx = 0; idx != VOICES; ++idx)
        {
          res[idx] = ChannelState();
          const uint_t MAX_ATTENUATION = 1024;
          if (attenuations[idx] < MAX_ATTENUATION)
          {
            const LevelType level(MAX_ATTENUATION - attenuations[idx], MAX_ATTENUATION);
            const uint_t band = Analyser.GetBandByPeriod(periods[idx]);
            res.push_back(ChannelState(band, level));
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
        : Params(params)
        , Target(target)
      {
        BaseChip::Reset();
      }

      virtual void RenderData(const typename ChipTraits::DataChunkType& src)
      {
        Buffer.Add(src);
      }

      virtual void Flush()
      {
        const typename ChipTraits::StampType tillTime = Buffer.GetTillTime();
        if (!(tillTime == typename ChipTraits::StampType(0)))
        {
          SynchronizeParameters();
          Sound::ChunkBuilder builder;
          builder.Reserve(Clock.SamplesTill(tillTime));
          RenderChunks(builder);
          Target->ApplyData(builder.GetResult());
        }
        Target->Flush();
      }

      virtual void Reset()
      {
        Params.Reset();
        Adapter.Reset();
        Clock.Reset();
        Buffer.Reset();
        SynchronizeParameters();
      }

      virtual void GetState(MultiChannelState& state) const
      {
        Adapter.GetState(state);
      }
    private:
      void SynchronizeParameters()
      {
        if (Params.IsChanged())
        {
          const uint64_t clockFreq = Params->ClockFreq();
          const uint_t sndFreq = Params->SoundFreq();
          Adapter.SetParams(clockFreq, sndFreq);
          Clock.SetFrequency(clockFreq, sndFreq);
        }
      }

      void RenderChunks(Sound::ChunkBuilder& builder)
      {
        for (const typename ChipTraits::DataChunkType* it = Buffer.GetBegin(), *lim = Buffer.GetEnd(); it != lim; ++it)
        {
          Adapter.WriteRegisters(*it);
          if (const uint_t samples = Clock.SamplesTill(it->TimeStamp))
          {
            Adapter.RenderSamples(samples, builder);
            Clock.SkipSamples(samples);
          }
        }
        Buffer.Reset();
      }
    private:
      Devices::Details::ParametersHelper<ChipParameters> Params;
      const Sound::Receiver::Ptr Target;
      typename ChipTraits::AdapterType Adapter;
      Devices::Details::ClockSource<typename ChipTraits::StampType> Clock;
      Devices::Details::ChunksCache<typename ChipTraits::DataChunkType, typename ChipTraits::StampType> Buffer;
    };
  }
}
}
