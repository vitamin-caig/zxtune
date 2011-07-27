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

    virtual void WriteFrame(uint_t framesPassed, const DataChunk& /*state*/, const DataChunk& update)
    {
      assert(framesPassed);

      Dump frame;
      frame.reserve(framesPassed * sizeof(uint16_t) + CountBits(update.Mask));
      std::back_insert_iterator<Dump> inserter(frame);
      //skipped frames
      std::fill_n(inserter, sizeof(uint16_t) * (framesPassed - 1), 0);
      *inserter = static_cast<Dump::value_type>(update.Mask & 0xff);
      *inserter = static_cast<Dump::value_type>(update.Mask >> 8);
      for (uint_t reg = 0, mask = update.Mask; mask; ++reg, mask >>= 1)
      {
        if (mask & 1)
        {
          *inserter = update.Data[reg];
        }
      }
      std::copy(frame.begin(), frame.end(), std::back_inserter(Data));
    }
  private:
    Dump Data;
  };
}

namespace Devices
{
  namespace AYM
  {
    Dumper::Ptr CreateZX50Dumper(const Time::Microseconds& frameDuration)
    {
      const FramedDumpBuilder::Ptr builder = boost::make_shared<ZX50DumpBuilder>();
      return CreateDumper(frameDuration, builder);
    }
  }
}
