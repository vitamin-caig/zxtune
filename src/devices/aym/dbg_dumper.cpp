/*
Abstract:
  Debug stream dumper imlementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dump_builder.h"
//common includes
#include <format.h>
#include <tools.h>
//boost includes
#include <boost/make_shared.hpp>
//std includes
#include <algorithm>

namespace
{
  using namespace Devices::AYM;
  
  const Char FRAME_NUMBER_FORMAT[] = {'%','1','$','0','5','u',' ','\0'};

  char HexSymbol(uint8_t sym)
  {
    return sym >= 10 ? 'A' + sym - 10 : '0' + sym;
  }

  class DebugDumpBuilder : public FramedDumpBuilder
  {
  public:
    DebugDumpBuilder()
      : FrameNumber()
    {
    }
    
    virtual void Initialize()
    {
      //static const std::string HEADER("##### 000102030405060708090a0b0c0d\n");
      static const std::string HEADER("000102030405060708090a0b0c0d\n");
      Data.assign(HEADER.begin(), HEADER.end());
      FrameNumber = 0;
    }

    virtual void GetResult(Dump& data) const
    {
      data = Data;
    }

    virtual void WriteFrame(uint_t framesPassed, const DataChunk& /*state*/, const DataChunk& update)
    {
      assert(framesPassed);
      for (uint_t skips = 0; skips < framesPassed - 1; ++skips)
      {
        AddNochangesMessage();
      }
      AddFrameNumber();
      for (uint_t reg = 0, mask = update.Mask; mask && reg < DataChunk::REG_LAST_AY; ++reg, mask >>= 1)
      {
        if (mask & 1)
        {
          const uint8_t data = update.Data[reg];
          AddData(data);
        }
        else
        {
          AddNoData();
        }
      }
      AddEndOfFrame();
    }
  private:
    void AddNochangesMessage()
    {
      AddFrameNumber();
      Data.push_back('=');
      AddEndOfFrame();
    }

    void AddFrameNumber()
    {
      const String number = Strings::Format(FRAME_NUMBER_FORMAT, FrameNumber);
      //std::copy(number.begin(), number.end(), std::back_inserter(Data));
    }

    void AddData(uint8_t data)
    {
      const uint8_t hiNibble = data >> 4;
      const uint8_t loNibble = data & 0x0f;
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
      ++FrameNumber;
    }
  private:
    Dump Data;
    uint_t FrameNumber;
  };
}

namespace Devices
{
  namespace AYM
  {
    Dumper::Ptr CreateDebugDumper(DumperParameters::Ptr params)
    {
      const FramedDumpBuilder::Ptr builder = boost::make_shared<DebugDumpBuilder>();
      return CreateDumper(params, builder);
    }
  }
}
