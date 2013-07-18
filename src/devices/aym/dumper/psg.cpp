/*
Abstract:
  PSG dumper imlementation

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

  // command codes
  enum Codes
  {
    INTERRUPT = 0xff,
    SKIP_INTS = 0xfe,
    END_MUS = 0xfd
  };

  class PSGDumpBuilder : public FramedDumpBuilder
  {
  public:
    virtual void Initialize()
    {
      static const uint8_t HEADER[] =
      {
        'P', 'S', 'G', 0x1a,
        0,//version
        0,//freq rate
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,//padding
        END_MUS
      };
      BOOST_STATIC_ASSERT(sizeof(HEADER) == 16 + 1);
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
      //SKIP_INTS code groups 4 skipped interrupts
      const uint_t SKIP_GROUP_SIZE = 4;
      const uint_t groupsSkipped = framesPassed / SKIP_GROUP_SIZE;
      const uint_t remainInts = framesPassed % SKIP_GROUP_SIZE;
      frame.reserve(groupsSkipped + remainInts + 2 * DataChunk::REG_LAST_AY + 1);
      std::back_insert_iterator<Dump> inserter(frame);
      if (groupsSkipped)
      {
        *inserter = SKIP_INTS;
        *inserter = static_cast<Dump::value_type>(groupsSkipped);
      }
      std::fill_n(inserter, remainInts, INTERRUPT);
      //store data
      for (uint_t reg = 0; reg < DataChunk::REG_LAST_AY; ++reg)
      {
        if (update.Has(reg))
        {
          *inserter = static_cast<Dump::value_type>(reg);
          *inserter = update[reg];
        }
      }
      *inserter = END_MUS;
      assert(!Data.empty());
      Data.pop_back();//delete limiter
      Data.reserve(Data.size() + frame.size());
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
    Dumper::Ptr CreatePSGDumper(DumperParameters::Ptr params)
    {
      const FramedDumpBuilder::Ptr builder = boost::make_shared<PSGDumpBuilder>();
      return CreateDumper(params, builder);
    }
  }
}
