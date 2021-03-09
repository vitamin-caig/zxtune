/**
* 
* @file
*
* @brief  Raw stream dumper implementation
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
#include <iterator>

namespace Devices::AYM
{
  class RawDumpBuilder : public FramedDumpBuilder
  {
  public:
    enum
    {
      NO_R13 = 0xff
    };
    
    void Initialize() override
    {
      Data.clear();
    }

    void GetResult(Dump& data) const override
    {
      data = Data;
    }

    void WriteFrame(uint_t framesPassed, const Registers& state, const Registers& update) override
    {
      assert(framesPassed);
      std::back_insert_iterator<Dump> inserter(Data);
      if (const uint_t toSkip = framesPassed - 1)
      {
        Dump dup;
        if (Data.empty())
        {
          dup.resize(Registers::TOTAL);
        }
        else
        {
          dup.assign(Data.end() - Registers::TOTAL, Data.end());
        }
        for (uint_t skips = 0; skips < toSkip; ++skips)
        {
          std::copy(dup.begin(), dup.end(), inserter);
        }
      }
      Registers fixedState(state);
      if (!update.Has(Registers::ENV))
      {
        fixedState[Registers::ENV] = NO_R13;
      }
      const uint8_t* const rawStart = &fixedState[Registers::TONEA_L];
      std::copy(rawStart, rawStart + Registers::TOTAL, inserter);
    }
  private:
    Dump Data;
  };

  FramedDumpBuilder::Ptr CreateRawDumpBuilder()
  {
    return MakePtr<RawDumpBuilder>();
  }

  Dumper::Ptr CreateRawStreamDumper(DumperParameters::Ptr params)
  {
    const FramedDumpBuilder::Ptr builder = CreateRawDumpBuilder();
    return CreateDumper(params, builder);
  }
}
