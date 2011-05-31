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
    FrameDumper(uint_t clocksPerFrame, FramedDumpBuilder::Ptr builder)
      : ClocksPerFrame(clocksPerFrame)
      , Builder(builder)
    {
      Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      DataChunk update;
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
            update.Mask |= mask;
            update.Data[reg] = newReg;
            State.Data[reg] = newReg;
          }
        }
      }
      if (!update.Mask)
      {
        return;
      }
      if (const uint_t framesPassed = static_cast<uint_t>((src.Tick - State.Tick) / ClocksPerFrame))
      {
        Builder->WriteFrame(framesPassed, State, update);
        State.Tick = src.Tick;
      }
      else
      {
        assert(!"Not a frame-divided AYM stream");
      }
    }

    virtual void Reset()
    {
      Builder->Initialize();
      State = DataChunk();
    }

    virtual void GetState(ChannelsState& state) const
    {
      std::fill(state.begin(), state.end(), ChanState());
    }

    virtual void GetDump(Dump& result) const
    {
      Builder->GetResult(result);
    }
  private:
    const uint_t ClocksPerFrame;
    const FramedDumpBuilder::Ptr Builder;
    DataChunk State;
  };
}

namespace Devices
{
  namespace AYM
  {
    Dumper::Ptr CreateDumper(uint_t clocksPerFrame, FramedDumpBuilder::Ptr builder)
    {
      return boost::make_shared<FrameDumper>(clocksPerFrame, builder);
    }
  }
}
