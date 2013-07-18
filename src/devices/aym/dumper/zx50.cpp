/*
Abstract:
  ZX50 dumper imlementation

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

  class ZX50DumpBuilder : public FramedDumpBuilder
  {
  public:
    virtual void Initialize()
    {
      static const Dump::value_type HEADER[] = 
      {
        'Z', 'X', '5', '0'
      };
      Data.assign(HEADER, ArrayEnd(HEADER));
    }

    virtual void GetResult(Dump& data) const
    {
      data = Data;
    }

    virtual void WriteFrame(uint_t framesPassed, const DataChunk::Registers& /*state*/, const DataChunk::Registers& update)
    {
      assert(framesPassed);

      Dump frame;
      frame.reserve(DataChunk::REG_LAST_AY);
      std::back_insert_iterator<Dump> inserter(frame);
      uint_t mask = 0;
      for (uint_t reg = 0; reg < DataChunk::REG_LAST_AY; ++reg)
      {
        if (update.Has(reg))
        {
          *inserter = update[reg];
          mask |= 1 << reg;
        }
      }
      //commit
      Data.reserve(Data.size() + framesPassed * sizeof(uint16_t) + DataChunk::REG_LAST_AY);
      std::back_insert_iterator<Dump> data(Data);
      //skipped frames
      std::fill_n(data, sizeof(uint16_t) * (framesPassed - 1), 0);
      *data = static_cast<Dump::value_type>(mask & 0xff);
      *data = static_cast<Dump::value_type>(mask >> 8);
      std::copy(frame.begin(), frame.end(), data);
    }
  private:
    Dump Data;
  };
}

namespace Devices
{
  namespace AYM
  {
    Dumper::Ptr CreateZX50Dumper(DumperParameters::Ptr params)
    {
      const FramedDumpBuilder::Ptr builder = boost::make_shared<ZX50DumpBuilder>();
      return CreateDumper(params, builder);
    }
  }
}
