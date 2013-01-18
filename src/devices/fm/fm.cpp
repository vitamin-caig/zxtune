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
      assert(YM2203);
      ::YM2203ResetChip(YM2203.get());
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

    void RenderSamples(uint_t count, Receiver& tgt)
    {
      assert(YM2203);
      if (count == 1)
      {
        YM2203SampleType buf = 0;
        ::YM2203UpdateOne(YM2203.get(), &buf, 1);
        tgt.ApplyData(ConvertToSample(buf));
      }
      else
      {
        std::vector<YM2203SampleType> buf(count);
        ::YM2203UpdateOne(YM2203.get(), &buf[0], count);
        std::for_each(buf.begin(), buf.end(), boost::bind(&Receiver::ApplyData, &tgt, boost::bind(&ConvertToSample, _1)));
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
        res[idx].Band = GetBandByFreq(freqs[idx]);
        res[idx].LevelInPercents = vols[idx] > 1024 ? 0 : (100 * (1024 - vols[idx])) / 1024;
      }
      return res;
    }
  private:
    static Sample ConvertToSample(YM2203SampleType level)
    {
      return static_cast<Sample>(int_t(level) + 32768);
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
    MameChip(ChipParameters::Ptr params, Receiver::Ptr target)
      : Params(params)
      , Target(target)
    {
    }

    virtual void RenderData(const DataChunk& src)
    {
      Buffer.push_back(src);
    }

    virtual void Flush()
    {
      ApplyParameters();
      std::for_each(Buffer.begin(), Buffer.end(), boost::bind(&MameChip::RenderSingleChunk, this, _1));
      Buffer.clear();
      Target->Flush();
    }

    virtual void Reset()
    {
      Render.Reset();
      Clock.Reset();
      Buffer.clear();
    }

    virtual void GetState(ChannelsState& state) const
    {
      state = Render.GetState();
    }
  private:
    void ApplyParameters()
    {
      const uint64_t clockFreq = Params->ClockFreq();
      const uint_t sndFreq = Params->SoundFreq();
      Render.SetParams(clockFreq, sndFreq);
      Clock.SetFrequency(sndFreq);
    }

    void RenderSingleChunk(const DataChunk& src)
    {
      Render.WriteRegisters(src.Data.begin(), src.Data.end());
      if (const uint_t outFrames = static_cast<uint_t>(Clock.GetTickAtTime(src.TimeStamp) - Clock.GetCurrentTick()))
      {
        Render.RenderSamples(outFrames, *Target);
        Clock.AdvanceTick(outFrames);
      }
    }
  private:
    const ChipParameters::Ptr Params;
    const Receiver::Ptr Target;
    ChipAdapter Render;
    Time::Oscillator<Stamp> Clock;
    std::vector<DataChunk> Buffer;
  };
}

namespace Devices
{
  namespace FM
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Receiver::Ptr target)
    {
      return boost::make_shared<MameChip>(params, target);
    }
  }
}
