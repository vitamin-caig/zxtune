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
//library includes
#include <sound/render_params.h>
//std includes
#include <algorithm>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::AYM;

  const uint8_t NO_R13 = 0xff;

  class RawStreamDumper : public Chip
  {
  public:
    explicit RawStreamDumper(Dump& data)
      : Data(data)
    {
      Reset();
    }

    virtual void RenderData(const Sound::RenderParameters& params,
                            const DataChunk& src,
                            Sound::MultichannelReceiver& /*dst*/)
    {
      if (const uint_t intsPassed = static_cast<uint_t>((src.Tick - CurChunk.Tick) / params.ClocksPerFrame()))
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
  private:
    void AddFrame()
    {
      std::copy(CurChunk.Data.begin(), CurChunk.Data.begin() + DataChunk::REG_ENV + 1, std::back_inserter(Data));
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
    Chip::Ptr CreateRawStreamDumper(Dump& data)
    {
      return Chip::Ptr(new RawStreamDumper(data));
    }
  }
}
