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

    virtual void WriteFrame(uint_t framesPassed, const DataChunk& state, const DataChunk& update)
    {
      assert(framesPassed);
      std::back_insert_iterator<Dump> inserter(Data);
      for (uint_t skips = 0; skips < framesPassed - 1; ++skips)
      {
        Dump dup;
        if (Data.size() >= DataChunk::REG_LAST)
        {
          dup.assign(Data.end() - DataChunk::REG_LAST, Data.end());
        }
        else
        {
          dup.resize(DataChunk::REG_LAST);
        }
        std::copy(dup.begin(), dup.end(), inserter);
      }
      DataChunk fixedState(state);
      if (0 == (update.Mask & (1 << DataChunk::REG_ENV)))
      {
        fixedState.Data[DataChunk::REG_ENV] = NO_R13;
      }
      std::copy(fixedState.Data.begin(), fixedState.Data.begin() + DataChunk::REG_LAST, inserter);
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

    Dumper::Ptr CreateRawStreamDumper(uint_t clocksPerFrame)
    {
      const FramedDumpBuilder::Ptr builder = CreateRawDumpBuilder();
      return CreateDumper(clocksPerFrame, builder);
    }
  }
}
