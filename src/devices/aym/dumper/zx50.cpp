/**
* 
* @file
*
* @brief  ZX50 dumper implementation
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
  class ZX50DumpBuilder : public FramedDumpBuilder
  {
  public:
    void Initialize() override
    {
      static const Dump::value_type HEADER[] = 
      {
        'Z', 'X', '5', '0'
      };
      Data.assign(HEADER, std::end(HEADER));
    }

    void GetResult(Dump& data) const override
    {
      data = Data;
    }

    void WriteFrame(uint_t framesPassed, const Registers& /*state*/, const Registers& update) override
    {
      assert(framesPassed);

      Dump frame;
      frame.reserve(Registers::TOTAL);
      std::back_insert_iterator<Dump> inserter(frame);
      uint_t mask = 0;
      for (Registers::IndicesIterator it(update); it; ++it)
      {
        *inserter = update[*it];
        mask |= 1 << *it;
      }
      //commit
      Data.reserve(Data.size() + framesPassed * sizeof(uint16_t) + Registers::TOTAL);
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

  Dumper::Ptr CreateZX50Dumper(DumperParameters::Ptr params)
  {
    const FramedDumpBuilder::Ptr builder = MakePtr<ZX50DumpBuilder>();
    return CreateDumper(params, builder);
  }
}
