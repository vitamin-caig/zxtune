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
#include <time_tools.h>
//library includes
#include <devices/fm.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  using namespace Devices::FM;

  typedef int YM2203SampleType;

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
      boost::array<int, VOICES> buf;
      ::YM2203GetAllTL(YM2203.get(), &buf[0]);
      for (uint_t idx = 0; idx != VOICES; ++idx)
      {
        res[idx] = ChanState('A' + idx);
        res[idx].Band = idx;//TODO
        res[idx].LevelInPercents = (100 * (1024 - buf[idx])) / 1024;
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
    Time::NanosecOscillator Clock;
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
