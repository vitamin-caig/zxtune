/*
Abstract:
  FM chips implementation

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
#include <devices/details/parameters_helper.h>
#include <sound/chunk_builder.h>
#include <time/oscillator.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace Devices::FM;

  typedef short YM2203SampleType;

  uint_t GetBandByFreq(uint_t freq)
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

  class ChipAdapter
  {
  public:
    ChipAdapter()
      : LastClockrate()
      , LastSoundFreq()
    {
    }

    void SetParams(uint64_t clock, uint_t sndFreq)
    {
      if (!YM2203 || clock != LastClockrate || LastSoundFreq != sndFreq)
      {
        YM2203 = ChipPtr(::YM2203Init(LastClockrate = clock, LastSoundFreq = sndFreq), &::YM2203Shutdown);
      }
    }

    void Reset()
    {
      if (YM2203.get())
      {
        ::YM2203ResetChip(YM2203.get());
      }
    }

    template<class It>
    void WriteRegisters(It begin, It end)
    {
      assert(YM2203);
      std::for_each(begin, end, boost::bind(&ChipAdapter::WriteRegister, this, _1));
    }

    void WriteRegister(const DataChunk::Register& reg)
    {
      assert(YM2203);
      ::YM2203Write(YM2203.get(), 0, reg.Index);
      ::YM2203Write(YM2203.get(), 1, reg.Value);
    }

    void RenderSamples(uint_t count, Sound::ChunkBuilder& tgt)
    {
      assert(YM2203);
      if (count == 1)
      {
        YM2203SampleType buf = 0;
        ::YM2203UpdateOne(YM2203.get(), &buf, 1);
        tgt.Add(ConvertToSample(buf));
      }
      else
      {
        std::vector<YM2203SampleType> buf(count);
        ::YM2203UpdateOne(YM2203.get(), &buf[0], count);
        Sound::Sample* const target = tgt.Allocate(count);
        std::transform(buf.begin(), buf.end(), target, boost::bind(&ConvertToSample, _1));
      }
    }

    ChannelsState GetState() const
    {
      assert(YM2203);
      ChannelsState res;
      boost::array<int, VOICES> vols;
      boost::array<int, VOICES> freqs;
      ::YM2203GetAllTL(YM2203.get(), &vols[0], &freqs[0]);
      for (uint_t idx = 0; idx != VOICES; ++idx)
      {
        res[idx] = ChanState('A' + idx);
        res[idx].Enabled = vols[idx] < 1024;
        res[idx].Band = GetBandByFreq(freqs[idx]);
        res[idx].LevelInPercents = vols[idx] > 1024 ? 0 : (100 * (1024 - vols[idx])) / 1024;
      }
      return res;
    }
  private:
    static Sound::Sample ConvertToSample(YM2203SampleType level)
    {
      return Sound::Sample(level, level);
    }
  private:
    typedef boost::shared_ptr<void> ChipPtr;
    ChipPtr YM2203;
    uint64_t LastClockrate;
    uint_t LastSoundFreq;
  };

  class MameChip : public Chip
  {
  public:
    MameChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
      : Params(params)
      , Target(target)
    {
      MameChip::Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      Buffer.Add(src);
    }

    virtual void Flush()
    {
      const Stamp tillTime = Buffer.GetTillTime();
      if (!(tillTime == Stamp(0)))
      {
        SynchronizeParameters();
        Sound::ChunkBuilder builder;
        builder.Reserve(GetSamplesTill(tillTime));
        RenderChunks(builder);
        Target->ApplyData(builder.GetResult());
      }
      Target->Flush();
    }

    virtual void Reset()
    {
      Params.Reset();
      Render.Reset();
      Clock.Reset();
      Buffer.Reset();
    }

    virtual void GetState(ChannelsState& state) const
    {
      state = Render.GetState();
    }
  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        const uint64_t clockFreq = Params->ClockFreq();
        const uint_t sndFreq = Params->SoundFreq();
        Render.SetParams(clockFreq, sndFreq);
        Clock.SetFrequency(sndFreq);
      }
    }

    uint_t GetSamplesTill(Stamp stamp) const
    {
      //TODO
      return Clock.GetCurrentTime() < stamp
        ? Clock.GetTickAtTime(stamp) - Clock.GetCurrentTick() + 2
        : 0;
    }

    void RenderChunks(Sound::ChunkBuilder& builder)
    {
      for (const DataChunk* it = Buffer.GetBegin(), *lim = Buffer.GetEnd(); it != lim; ++it)
      {
        Render.WriteRegisters(it->Data.begin(), it->Data.end());
        if (const uint_t samples = GetSamplesTill(it->TimeStamp))
        {
          Render.RenderSamples(samples, builder);
          Clock.AdvanceTick(samples);
        }
      }
      Buffer.Reset();
    }
  private:
    Devices::Details::ParametersHelper<ChipParameters> Params;
    const Sound::Receiver::Ptr Target;
    ChipAdapter Render;
    Time::Oscillator<Stamp> Clock;
    Devices::Details::ChunksCache<DataChunk, Stamp> Buffer;
  };
}

namespace Devices
{
  namespace FM
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      return boost::make_shared<MameChip>(params, target);
    }
  }
}
