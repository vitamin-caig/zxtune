/*
Abstract:
  AY dump builder implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dump_builder.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace Devices::AYM;

  class FrameDumper : public Dumper
  {
  public:
    FrameDumper(const Time::Microseconds& frameDuration, FramedDumpBuilder::Ptr builder)
      : FrameDuration(frameDuration)
      , Builder(builder)
    {
      Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      const Time::Nanoseconds::ValueType nextTime = src.TimeStamp.Get();
      FlushFinishedFrame(nextTime);
      //calculate difference
      for (uint_t reg = 0, mask = 1; reg < DataChunk::REG_LAST; ++reg, mask <<= 1)
      {
        // apply chunk if some data changed or env register (even if not changed)
        if (src.Mask & mask)
        {
          const uint8_t oldReg = State.Data[reg];
          const uint8_t newReg = src.Data[reg];
          if (reg == DataChunk::REG_ENV || oldReg != newReg)
          {
            Update.Mask |= mask;
            Update.Data[reg] = newReg;
            State.Data[reg] = newReg;
            Update.TimeStamp = src.TimeStamp;
          }
        }
      }
    }

    virtual void Flush()
    {
    }

    virtual void Reset()
    {
      Builder->Initialize();
      State = DataChunk();
      Update = DataChunk();
    }

    virtual void GetDump(Dump& result) const
    {
      Builder->GetResult(result);
    }
  private:
    void FlushFinishedFrame(Time::Nanoseconds::ValueType nextTime)
    {
      const Time::Nanoseconds::ValueType curTime = State.TimeStamp.Get();
      const Time::Nanoseconds::ValueType frameDuration = FrameDuration.Get();
      if (const uint_t framesPassed = static_cast<uint_t>((nextTime - curTime) / frameDuration))
      {
        Builder->WriteFrame(framesPassed, State, Update);
        Update = DataChunk();
        State.TimeStamp += Time::Nanoseconds(framesPassed * frameDuration);
      }
    }
  private:
    const Time::Nanoseconds FrameDuration;
    const FramedDumpBuilder::Ptr Builder;
    DataChunk State;
    DataChunk Update;
  };
}

namespace Devices
{
  namespace AYM
  {
    Dumper::Ptr CreateDumper(const Time::Microseconds& frameDuration, FramedDumpBuilder::Ptr builder)
    {
      return boost::make_shared<FrameDumper>(frameDuration, builder);
    }
  }
}
