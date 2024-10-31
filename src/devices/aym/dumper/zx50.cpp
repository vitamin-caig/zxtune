/**
 *
 * @file
 *
 * @brief  ZX50 dumper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "devices/aym/dumper/dump_builder.h"

#include "binary/data_builder.h"

#include "make_ptr.h"

namespace Devices::AYM
{
  class ZX50DumpBuilder : public FramedDumpBuilder
  {
  public:
    void Initialize() override
    {
      static const uint8_t HEADER[] = {'Z', 'X', '5', '0'};
      Data.Add(Binary::View{HEADER, sizeof(HEADER)});
    }

    Binary::Data::Ptr GetResult() override
    {
      return Data.CaptureResult();
    }

    void WriteFrame(uint_t framesPassed, const Registers& /*state*/, const Registers& update) override
    {
      const auto sizeBefore = Data.Size();
      const auto skips = sizeof(uint16_t) * (framesPassed - 1);
      auto* mask = static_cast<uint8_t*>(Data.Allocate(skips + sizeof(uint16_t) + Registers::TOTAL)) + skips;
      auto* target = mask + sizeof(uint16_t);
      uint_t bitmask = 0;
      for (Registers::IndicesIterator it(update); it; ++it)
      {
        *target++ = update[*it];
        bitmask |= 1 << *it;
      }
      mask[0] = static_cast<uint8_t>(bitmask & 0xff);
      mask[1] = static_cast<uint8_t>(bitmask >> 8);
      const auto frameSize = target - mask;
      Data.Resize(sizeBefore + skips + frameSize);
    }

  private:
    Binary::DataBuilder Data;
  };

  Dumper::Ptr CreateZX50Dumper(const DumperParameters& params)
  {
    auto builder = MakePtr<ZX50DumpBuilder>();
    return CreateDumper(params, std::move(builder));
  }
}  // namespace Devices::AYM
