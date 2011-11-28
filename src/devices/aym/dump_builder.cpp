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

  class RenderState
  {
  public:
    void Update(const DataChunk& delta)
    {
      if (0 == delta.Mask)
      {
        return;
      }
      for (uint_t reg = 0, mask = 1; reg < DataChunk::REG_LAST; ++reg, mask <<= 1)
      {
        // apply chunk if some data changed or env register (even if not changed)
        if (delta.Mask & mask)
        {
          const uint8_t oldReg = Actual.Data[reg];
          const uint8_t newReg = delta.Data[reg];
          if (reg == DataChunk::REG_ENV || oldReg != newReg)
          {
            Actual.Mask |= mask;
            Actual.Data[reg] = newReg;
          }
        }
      }
    }

    const DataChunk& GetBase() const
    {
      return Base;
    }

    DataChunk GetDelta() const
    {
      DataChunk delta;
      for (uint_t reg = 0, mask = 1; reg < DataChunk::REG_LAST; ++reg, mask <<= 1)
      {
        if (Actual.Mask & mask)
        {
          const uint8_t oldReg = Base.Data[reg];
          const uint8_t newReg = Actual.Data[reg];
          if (reg == DataChunk::REG_ENV || oldReg != newReg)
          {
            delta.Data[reg] = newReg;
            delta.Mask |= mask;
          }
        }
      }
      return delta;
    }

    void Commit(const Time::Nanoseconds& stamp)
    {
      Actual.Mask = 0;
      Base = Actual;
      Base.TimeStamp = stamp;
    }
  private:
    DataChunk Base;
    DataChunk Actual;
  };

  class FrameDumper : public Dumper
  {
  public:
    FrameDumper(const Time::Microseconds& frameDuration, FramedDumpBuilder::Ptr builder)
      : FrameDuration(frameDuration)
      , Builder(builder)
      , State()
      , FramesToSkip(0)
    {
      Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      Buffer.push_back(src);
    }

    virtual void Flush()
    {
      Time::Nanoseconds nextFrame = State.GetBase().TimeStamp;
      nextFrame += FrameDuration;
      for (std::vector<DataChunk>::iterator it = Buffer.begin(), lim = Buffer.end(); it != lim; ++it)
      {
        if (it->TimeStamp < nextFrame)
        {
          State.Update(*it);
        }
        else 
        {
          ++FramesToSkip;
          const DataChunk& delta = State.GetDelta();
          if (delta.Mask)
          {
            Builder->WriteFrame(FramesToSkip, State.GetBase(), delta);
            FramesToSkip = 0;
          }
          State.Commit(nextFrame);
          nextFrame += FrameDuration;
        }
      }
      Buffer.clear();
    }

    virtual void Reset()
    {
      Builder->Initialize();
      Buffer.clear();
      State = RenderState();
      FramesToSkip = 0;
    }

    virtual void GetDump(Dump& result) const
    {
      Builder->GetResult(result);
    }
  private:
    const Time::Nanoseconds FrameDuration;
    const FramedDumpBuilder::Ptr Builder;
    std::vector<DataChunk> Buffer;
    RenderState State;
    uint_t FramesToSkip;
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
