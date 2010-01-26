/*
Abstract:
  PSG dumper imlementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <tools.h>
#include <core/devices/aym.h>
#include <sound/render_params.h>

#include <algorithm>
#include <iterator>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::AYM;
  
  enum Codes
  {
    INTERRUPT = 0xff,
    SKIP_INTS = 0xfe,
    END_MUS = 0xfd
  };
  
  template<class T>
  unsigned CountBits(T val)
  {
    unsigned res = 0;
    while (val)
    {
      if (val & 1)
      {
        ++res;
      }
      val >>= 1;
    }
    return res;
  }
  
  class PSGDumper : public Chip
  {
  public:
    explicit PSGDumper(Dump& data)
      : Data(data)
    {
      Reset();
    }
    
    virtual void RenderData(const Sound::RenderParameters& params,
                            const DataChunk& src,
                            Sound::MultichannelReceiver& /*dst*/)
    {
      if (0 == (src.Mask & DataChunk::ALL_REGISTERS))
      {
        return;
      }
      //check for difference
      {
        unsigned mask = src.Mask & DataChunk::ALL_REGISTERS;
        for (unsigned reg = 0; mask; ++reg, mask >>= 1)
        {
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
      if (const unsigned intsPassed = static_cast<unsigned>((src.Tick - CurChunk.Tick) / params.ClocksPerFrame()))
      {
        Dump frame;
        std::back_insert_iterator<Dump> inserter(frame);
        frame.reserve(intsPassed / 4 + (intsPassed % 4) + 2 * CountBits(src.Mask & DataChunk::ALL_REGISTERS) + 1);
        if (const unsigned fourSkips = intsPassed / 4)
        {
          *inserter = SKIP_INTS;
          *inserter = fourSkips;
        }
        std::fill_n(inserter, intsPassed % 4, INTERRUPT);
        unsigned mask = src.Mask;
        for (unsigned reg = 0; mask; ++reg, mask >>= 1)
        {
          if ((mask & 1) && src.Data[reg] != CurChunk.Data[reg])
          {
            *inserter = reg;
            *inserter = src.Data[reg];
            CurChunk.Data[reg] = src.Data[reg];
          }
        }
        *inserter = END_MUS;
        Data.pop_back();//delete limiter
        std::copy(frame.begin(), frame.end(), std::back_inserter(Data));
        CurChunk.Tick = src.Tick;
      }
    }

    virtual void GetState(Module::Analyze::ChannelsState& state) const
    {
      state.clear();
    }

      /// reset internal state to initial
    virtual void Reset()
    {
      static const uint8_t HEADER[] = {
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
    Dump& Data;
    DataChunk CurChunk;
  };
}

namespace ZXTune
{
  namespace AYM
  {
    Chip::Ptr CreatePSGDumper(Dump& data)
    {
      return Chip::Ptr(new PSGDumper(data));
    }
  }
}
