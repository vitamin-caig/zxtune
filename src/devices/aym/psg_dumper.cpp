/*
Abstract:
  PSG dumper imlementation

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

  // command codes
  enum Codes
  {
    INTERRUPT = 0xff,
    SKIP_INTS = 0xfe,
    END_MUS = 0xfd
  };
  
  class PSGDumper : public Chip
  {
  public:
    PSGDumper(uint_t clocksPerFrame, Dump& data)
      : ClocksPerFrame(clocksPerFrame)
      , Data(data)
    {
      Reset();
    }
    
    virtual void RenderData(const ZXTune::Sound::RenderParameters& /*params*/,
                            const DataChunk& src)
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
        std::back_insert_iterator<Dump> inserter(frame);
        //SKIP_INTS code groups 4 skipped interrupts
        const uint_t SKIP_GROUP_SIZE = 4;
        frame.reserve(intsPassed / SKIP_GROUP_SIZE + (intsPassed % SKIP_GROUP_SIZE) + 2 * CountBits(src.Mask & DataChunk::MASK_ALL_REGISTERS) + 1);
        if (const uint_t fourSkips = intsPassed / SKIP_GROUP_SIZE)
        {
          *inserter = SKIP_INTS;
          *inserter = static_cast<Dump::value_type>(fourSkips);
        }
        //fill remain interrupts
        std::fill_n(inserter, intsPassed % SKIP_GROUP_SIZE, INTERRUPT);
        //store data
        for (uint_t reg = 0, mask = src.Mask & DataChunk::MASK_ALL_REGISTERS; mask; ++reg, mask >>= 1)
        {
          if ((mask & 1) && (reg == DataChunk::REG_ENV || src.Data[reg] != CurChunk.Data[reg]))
          {
            *inserter = static_cast<Dump::value_type>(reg);
            *inserter = src.Data[reg];
            CurChunk.Data[reg] = src.Data[reg];
          }
        }
        *inserter = END_MUS;
        assert(!Data.empty());
        Data.pop_back();//delete limiter
        std::copy(frame.begin(), frame.end(), std::back_inserter(Data));
        CurChunk.Tick = src.Tick;
      }
    }

    virtual void GetState(ChannelsState& state) const
    {
      std::fill(state.begin(), state.end(), ChanState());
    }

      /// reset internal state to initial
    virtual void Reset()
    {
      static const uint8_t HEADER[] =
      {
        'P', 'S', 'G', 0x1a,
        0,//version
        0,//freq rate
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0//padding
      };
      BOOST_STATIC_ASSERT(sizeof(HEADER) == 16);
      Data.resize(sizeof(HEADER) + 1);
      *std::copy(HEADER, ArrayEnd(HEADER), Data.begin()) = END_MUS;
      CurChunk = DataChunk();
    }

  private:
    const uint_t ClocksPerFrame;
    Dump& Data;
    DataChunk CurChunk;
  };
}

namespace Devices
{
  namespace AYM
  {
    Chip::Ptr CreatePSGDumper(uint_t clocksPerFrame, Dump& data)
    {
      return Chip::Ptr(new PSGDumper(clocksPerFrame, data));
    }
  }
}
