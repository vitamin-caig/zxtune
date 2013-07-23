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
    //TODO: extract
    inline uint_t GetBandByFreq(uint_t freq)
    {
      //table in Hz * FREQ_MULTIPLIER
      static const uint_t FREQ_TABLE[] =
      {
        //octave1
        3270,   3465,   3671,   3889,   4120,   4365,   4625,   4900,   5191,   5500,   5827,   6173,
        //octave2
        6541,   6929,   7342,   7778,   8241,   8730,   9250,   9800,  10382,  11000,  11654,  12346,
        //octave3
        13082,  13858,  14684,  15556,  16482,  17460,  18500,  19600,  20764,  22000,  23308,  24692,
        //octave4
        26164,  27716,  29368,  31112,  32964,  34920,  37000,  39200,  41528,  44000,  46616,  49384,
        //octave5
        52328,  55432,  58736,  62224,  65928,  69840,  74000,  78400,  83056,  88000,  93232,  98768,
        //octave6
        104650, 110860, 117470, 124450, 131860, 139680, 148000, 156800, 166110, 176000, 186460, 197540,
        //octave7
        209310, 221720, 234940, 248890, 263710, 279360, 296000, 313600, 332220, 352000, 372930, 395070,
        //octave8
        418620, 443460, 469890, 497790, 527420, 558720, 592000, 627200, 664450, 704000, 745860, 790140,
        //octave9
        837200, 886980, 939730, 995610,1054800,1117500,1184000,1254400,1329000,1408000,1491700,1580400
      };
      const uint_t maxBand = static_cast<uint_t>(ArraySize(FREQ_TABLE) - 1);
      const uint_t currentBand = static_cast<uint_t>(std::lower_bound(FREQ_TABLE, ArrayEnd(FREQ_TABLE), freq) - FREQ_TABLE);
      return std::min(currentBand, maxBand);
    }

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

      static void ConvertState(const int* vols, const int* freqs, ChanState* res, uint_t baseIdx)
      {
        for (uint_t idx = 0; idx != VOICES; ++idx)
        {
          res[idx] = ChanState('A' + idx + baseIdx);
          if ((res[idx].Enabled = vols[idx] < 1024))
          {
            res[idx].LevelInPercents = (100 * (1024 - vols[idx])) / 1024;
            res[idx].Band = GetBandByFreq(freqs[idx]);
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
      }

      virtual void GetState(ChannelsState& state) const
      {
        state = Adapter.GetState();
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
