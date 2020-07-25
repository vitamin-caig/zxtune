/**
* 
* @file
*
* @brief  Debug stream dumper implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "devices/aym/dumper/dump_builder.h"
//common includes
#include <make_ptr.h>
//std includes
#include <algorithm>

namespace
{
  inline char HexSymbol(uint8_t sym)
  {
    return sym >= 10 ? 'A' + sym - 10 : '0' + sym;
  }
}

namespace Devices
{
namespace AYM
{
  class DebugDumpBuilder : public FramedDumpBuilder
  {
  public:
    DebugDumpBuilder()
      : FrameNumber()
    {
    }
    
    void Initialize() override
    {
      static const std::string HEADER("000102030405060708090a0b0c0d\n");
      Data.assign(HEADER.begin(), HEADER.end());
      FrameNumber = 0;
    }

    void GetResult(Dump& data) const override
    {
      data = Data;
    }

    void WriteFrame(uint_t framesPassed, const Registers& /*state*/, const Registers& update) override
    {
      assert(framesPassed);
      for (uint_t skips = 0; skips < framesPassed - 1; ++skips)
      {
        AddNochangesMessage();
      }
      Dump str(Registers::TOTAL * 2, ' ');
      for (Registers::IndicesIterator it(update); it; ++it)
      {
        const uint8_t val = update[*it];
        str[*it * 2 + 0] = HexSymbol(val >> 4);
        str[*it * 2 + 1] = HexSymbol(val & 15);
      }
      AddData(str);
      AddEndOfFrame();
    }
  private:
    void AddNochangesMessage()
    {
      Data.push_back('=');
      AddEndOfFrame();
    }

    void AddData(const Dump& str)
    {
      std::copy(str.begin(), str.end(), std::back_inserter(Data));
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

  Dumper::Ptr CreateDebugDumper(DumperParameters::Ptr params)
  {
    const FramedDumpBuilder::Ptr builder = MakePtr<DebugDumpBuilder>();
    return CreateDumper(params, builder);
  }
}
}
