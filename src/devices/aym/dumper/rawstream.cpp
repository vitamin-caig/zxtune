/*
Abstract:
  Raw stream dumper imlementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dump_builder.h"
//boost includes
#include <boost/make_shared.hpp>
//std includes
#include <algorithm>
#include <iterator>

namespace
{
  using namespace Devices::AYM;

  const uint8_t NO_R13 = 0xff;

  class RawDumpBuilder : public FramedDumpBuilder
  {
  public:
    virtual void Initialize()
    {
      Data.clear();
    }

    virtual void GetResult(Dump& data) const
    {
      data = Data;
    }

    virtual void WriteFrame(uint_t framesPassed, const Registers& state, const Registers& update)
    {
      assert(framesPassed);
      Data.reserve(Data.size() + framesPassed * Registers::TOTAL);
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
}

namespace Devices
{
  namespace AYM
  {
    FramedDumpBuilder::Ptr CreateRawDumpBuilder()
    {
      return boost::make_shared<RawDumpBuilder>();
    }

    Dumper::Ptr CreateRawStreamDumper(DumperParameters::Ptr params)
    {
      const FramedDumpBuilder::Ptr builder = CreateRawDumpBuilder();
      return CreateDumper(params, builder);
    }
  }
}
