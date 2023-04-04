/**
 *
 * @file
 *
 * @brief  PSG dumper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "devices/aym/dumper/dump_builder.h"
// common includes
#include <make_ptr.h>
// library includes
#include <binary/data_builder.h>
// std includes
#include <algorithm>

namespace Devices::AYM
{
  class PSGDumpBuilder : public FramedDumpBuilder
  {
  public:
    enum CommandCodes
    {
      INTERRUPT = 0xff,
      SKIP_INTS = 0xfe,
      END_MUS = 0xfd
    };

    void Initialize() override
    {
      static const uint8_t HEADER[] = {'P',    'S', 'G', 0x1a,
                                       0,                                         // version
                                       0,                                         // freq rate
                                       0,      0,   0,   0,    0, 0, 0, 0, 0, 0,  // padding
                                       END_MUS};
      static_assert(sizeof(HEADER) == 16 + 1, "Invalid header layout");
      Data.Add(Binary::View{HEADER, sizeof(HEADER)});
    }

    Binary::Data::Ptr GetResult() override
    {
      return Data.CaptureResult();
    }

    void WriteFrame(uint_t framesPassed, const Registers& /*state*/, const Registers& update) override
    {
      const auto sizeBefore = Data.Size();
      // SKIP_INTS code groups 4 skipped interrupts
      const uint_t SKIP_GROUP_SIZE = 4;
      const uint_t groupsSkipped = framesPassed / SKIP_GROUP_SIZE;
      const uint_t remainInts = framesPassed % SKIP_GROUP_SIZE;
      auto* end = static_cast<uint8_t*>(Data.Allocate((groupsSkipped ? 2 : 0) + remainInts + 2 * Registers::TOTAL + 1));
      auto* target = end - 1;  // overwrite limiter
      if (groupsSkipped)
      {
        *target++ = SKIP_INTS;
        *target++ = static_cast<uint8_t>(groupsSkipped);
      }
      target = std::fill_n(target, remainInts, INTERRUPT);
      // store data
      for (Registers::IndicesIterator it(update); it; ++it)
      {
        *target++ = static_cast<uint8_t>(*it);
        *target++ = update[*it];
      }
      *target++ = END_MUS;
      const auto frameSize = target - end;
      Data.Resize(sizeBefore + frameSize);
    }

  private:
    Binary::DataBuilder Data;
  };

  Dumper::Ptr CreatePSGDumper(const DumperParameters& params)
  {
    auto builder = MakePtr<PSGDumpBuilder>();
    return CreateDumper(params, std::move(builder));
  }
}  // namespace Devices::AYM
