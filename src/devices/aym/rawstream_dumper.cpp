/*
Abstract:
  Raw stream dumper imlementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <devices/aym.h>
//common includes
#include <tools.h>
//std includes
#include <algorithm>

namespace
{
  using namespace Devices::AYM;

  const uint8_t NO_R13 = 0xff;

  class RawStreamDumper : public Dumper
  {
  public:
    explicit RawStreamDumper(uint_t clocksPerFrame)
      : ClocksPerFrame(clocksPerFrame)
    {
      Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      if (const uint_t intsPassed = static_cast<uint_t>((src.Tick - CurChunk.Tick) / ClocksPerFrame))
      {
        for (uint_t reg = 0, mask = src.Mask & DataChunk::MASK_ALL_REGISTERS; mask; ++reg, mask >>= 1)
        {
          if ((mask & 1) && (reg == DataChunk::REG_ENV || src.Data[reg] != CurChunk.Data[reg]))
          {
            const uint_t data = src.Data[reg];
            CurChunk.Data[reg] = data;
          }
        }
        if (0 == (src.Mask & (1 << DataChunk::REG_ENV)))
        {
          CurChunk.Data[DataChunk::REG_ENV] = NO_R13;
        }
        CurChunk.Tick = src.Tick;
        AddFrame();
      }
    }

    virtual void GetState(ChannelsState& state) const
    {
      std::fill(state.begin(), state.end(), ChanState());
    }

    virtual void Reset()
    {
      Data.clear();
      CurChunk = DataChunk();
    }

    virtual void GetDump(Dump& result) const
    {
      result.assign(Data.begin(), Data.end());
    }
  private:
    void AddFrame()
    {
      std::copy(CurChunk.Data.begin(), CurChunk.Data.begin() + DataChunk::REG_ENV + 1, std::back_inserter(Data));
    }
  private:
    const uint_t ClocksPerFrame;
    Dump Data;
    DataChunk CurChunk;
  };
}

namespace Devices
{
  namespace AYM
  {
    Dumper::Ptr CreateRawStreamDumper(uint_t clocksPerFrame)
    {
      return Dumper::Ptr(new RawStreamDumper(clocksPerFrame));
    }
  }
}
