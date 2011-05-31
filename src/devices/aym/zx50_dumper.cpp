/*
Abstract:
  ZX50 dumper imlementation

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
#include <iterator>

namespace
{
  using namespace Devices::AYM;
  
  class ZX50Dumper : public Dumper
  {
  public:
    explicit ZX50Dumper(uint_t clocksPerFrame)
      : ClocksPerFrame(clocksPerFrame)
    {
      Reset();
    }
    
    virtual void RenderData(const DataChunk& src)
    {
      //no data check
      if (0 == (src.Mask & DataChunk::MASK_ALL_REGISTERS))
      {
        return;
      }
      //check for difference
      {
        uint_t mask = src.Mask & DataChunk::MASK_ALL_REGISTERS;
        for (uint_t reg = 0; mask; ++reg, mask >>= 1)
        {
          // apply chunk if some data changed or env register (even if not changed)
          if ((mask & 1) && (reg == DataChunk::REG_ENV || src.Data[reg] != CurChunk.Data[reg]))
          {
            break;
          }
        }
        if (!mask)
        {
          return;//no differences
        }
      }
      if (const uint_t intsPassed = static_cast<uint_t>((src.Tick - CurChunk.Tick) / ClocksPerFrame))
      {
        Dump frame;
        frame.reserve(intsPassed * sizeof(uint16_t) + CountBits(src.Mask));
        std::back_insert_iterator<Dump> inserter(frame);
        //skipped frames
        std::fill_n(inserter, sizeof(uint16_t) * (intsPassed - 1), 0);
        *inserter = static_cast<Dump::value_type>(src.Mask & 0xff);
        *inserter = static_cast<Dump::value_type>(src.Mask >> 8);
        for (uint_t reg = 0, mask = src.Mask & DataChunk::MASK_ALL_REGISTERS; mask; ++reg, mask >>= 1)
        {
          if ((mask & 1) && (reg == DataChunk::REG_ENV || src.Data[reg] != CurChunk.Data[reg]))
          {
            *inserter = src.Data[reg];
            CurChunk.Data[reg] = src.Data[reg];
          }
        }
        std::copy(frame.begin(), frame.end(), std::back_inserter(Data));
        CurChunk.Tick = src.Tick;
      }
    }

    virtual void GetState(ChannelsState& state) const
    {
      std::fill(state.begin(), state.end(), ChanState());
    }

    virtual void Reset()
    {
      static const Dump::value_type HEADER[] = 
      {
        'Z', 'X', '5', '0'
      };
      Data.assign(HEADER, ArrayEnd(HEADER));
      CurChunk = DataChunk();
    }

    virtual void GetDump(Dump& result) const
    {
      result.assign(Data.begin(), Data.end());
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
    Dumper::Ptr CreateZX50Dumper(uint_t clocksPerFrame)
    {
      return Dumper::Ptr(new ZX50Dumper(clocksPerFrame));
    }
  }
}
