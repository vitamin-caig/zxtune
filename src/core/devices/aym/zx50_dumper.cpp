/*
Abstract:
  ZX50 dumper imlementation

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
  
  class ZX50Dumper : public Chip
  {
  public:
    explicit ZX50Dumper(Dump& data)
      : Data(data)
    {
      Reset();
    }
    
    virtual void RenderData(const Sound::RenderParameters& params,
                            const DataChunk& src,
                            Sound::MultichannelReceiver& /*dst*/)
    {
      if (0 == (src.Mask & DataChunk::MASK_ALL_REGISTERS))
      {
        return;
      }
      //check for difference
      {
        uint_t mask = src.Mask & DataChunk::MASK_ALL_REGISTERS;
        for (uint_t reg = 0; mask; ++reg, mask >>= 1)
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
      if (const uint_t intsPassed = static_cast<uint_t>((src.Tick - CurChunk.Tick) / params.ClocksPerFrame()))
      {
        Dump frame;
        frame.reserve(intsPassed * sizeof(uint16_t) + CountBits(src.Mask));
        std::back_insert_iterator<Dump> inserter(frame);
        //skipped frames
        std::fill_n(inserter, sizeof(uint16_t) * (intsPassed - 1), 0);
        *inserter = static_cast<Dump::value_type>(src.Mask & 0xff);
        *inserter = static_cast<Dump::value_type>(src.Mask >> 8);
        for (uint_t reg = 0, mask = src.Mask; mask; ++reg, mask >>= 1)
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

    virtual void GetState(Module::Analyze::ChannelsState& state) const
    {
      state.clear();
    }

      /// reset internal state to initial
    virtual void Reset()
    {
      Data.clear();
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
    Chip::Ptr CreateZX50Dumper(Dump& data)
    {
      return Chip::Ptr(new ZX50Dumper(data));
    }
  }
}
