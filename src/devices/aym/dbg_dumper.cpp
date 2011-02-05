/*
Abstract:
  Debug stream dumper imlementation

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

  char HexSymbol(uint_t sym)
  {
    return sym >= 10 ? 'A' + sym - 10 : '0' + sym;
  }

  class DebugDumper : public Chip
  {
  public:
    explicit DebugDumper(Dump& data)
      : Data(data)
    {
      Reset();
    }

    virtual void RenderData(const Sound::RenderParameters& params,
                            const DataChunk& src,
                            Sound::MultichannelReceiver& /*dst*/)
    {
      //no data check
      if (0 == (src.Mask & DataChunk::MASK_ALL_REGISTERS))
      {
        CurChunk.Tick = src.Tick;
        AddNodataMessage();
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
          CurChunk.Tick = src.Tick;
          AddNochangesMessage();
          return;//no differences
        }
      }
      if (const uint_t intsPassed = static_cast<uint_t>((src.Tick - CurChunk.Tick) / params.ClocksPerFrame()))
      {
        assert(intsPassed);
        for (uint_t reg = 0, mask = src.Mask & DataChunk::MASK_ALL_REGISTERS; mask; ++reg, mask >>= 1)
        {
          if ((mask & 1) && (reg == DataChunk::REG_ENV || src.Data[reg] != CurChunk.Data[reg]))
          {
            const uint_t data = src.Data[reg];
            AddData(data);
            CurChunk.Data[reg] = data;
          }
          else
          {
            AddNoData();
          }
        }
        CurChunk.Tick = src.Tick;
        AddEndOfFrame();
      }
    }

    virtual void GetState(ChannelsState& state) const
    {
      std::fill(state.begin(), state.end(), ChanState());
    }

    virtual void Reset()
    {
      static const std::string HEADER("000102030405060708090a0b0c0d\n");
      Data.assign(HEADER.begin(), HEADER.end());
      CurChunk = DataChunk();
    }
  private:
    void AddNodataMessage()
    {
      Data.push_back('0');
      AddEndOfFrame();
    }

    void AddNochangesMessage()
    {
      Data.push_back('=');
      AddEndOfFrame();
    }

    void AddData(uint_t data)
    {
      const uint_t hiNibble = data >> 4;
      const uint_t loNibble = data & 0x0f;
      Data.push_back(HexSymbol(hiNibble));
      Data.push_back(HexSymbol(loNibble));
    }

    void AddNoData()
    {
      Data.push_back(' ');
      Data.push_back(' ');
    }

    void AddEndOfFrame()
    {
      Data.push_back('\n');
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
    Chip::Ptr CreateDebugDumper(Dump& data)
    {
      return Chip::Ptr(new DebugDumper(data));
    }
  }
}
