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
//common includes
#include <tools.h>
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

    virtual void WriteFrame(uint_t framesPassed, const DataChunk::Registers& state, const DataChunk::Registers& update)
    {
      assert(framesPassed);
      Data.reserve(Data.size() + framesPassed * DataChunk::REG_LAST_AY);
      std::back_insert_iterator<Dump> inserter(Data);
      if (const uint_t toSkip = framesPassed - 1)
      {
        Dump dup;
        if (Data.size() >= DataChunk::REG_LAST_AY)
        {
          dup.assign(Data.end() - DataChunk::REG_LAST_AY, Data.end());
        }
        else
        {
          dup.resize(DataChunk::REG_LAST_AY);
        }
        for (uint_t skips = 0; skips < toSkip; ++skips)
        {
          std::copy(dup.begin(), dup.end(), inserter);
        }
      }
      DataChunk::Registers fixedState(state);
      if (!update.Has(DataChunk::REG_ENV))
      {
        fixedState[DataChunk::REG_ENV] = NO_R13;
      }
      const uint8_t* const rawStart = &fixedState[0];
      std::copy(rawStart, rawStart + DataChunk::REG_LAST_AY, inserter);
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
