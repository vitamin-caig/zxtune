/**
 *
 * @file
 *
 * @brief  Raw stream dumper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "devices/aym/dumper/dump_builder.h"

#include <binary/data_builder.h>

#include <make_ptr.h>

#include <algorithm>

namespace Devices::AYM
{
  class RawDumpBuilder : public FramedDumpBuilder
  {
  public:
    enum
    {
      NO_R13 = 0xff
    };

    void Initialize() override {}

    Binary::Data::Ptr GetResult() override
    {
      return Data.CaptureResult();
    }

    void WriteFrame(uint_t framesPassed, const Registers& state, const Registers& update) override
    {
      if (const uint_t toSkip = framesPassed - 1)
      {
        const auto size = Data.Size();
        auto* target = static_cast<uint8_t*>(Data.Allocate(Registers::TOTAL * toSkip));
        if (size >= Registers::TOTAL)
        {
          std::copy_n(&Data.Get<uint8_t>(size - Registers::TOTAL), Registers::TOTAL * toSkip, target);
        }
      }
      Registers fixedState(state);
      if (!update.Has(Registers::ENV))
      {
        fixedState[Registers::ENV] = NO_R13;
      }
      const uint8_t* const rawStart = &fixedState[Registers::TONEA_L];
      Data.Add(Binary::View{rawStart, Registers::TOTAL});
    }

  private:
    Binary::DataBuilder Data;
  };

  FramedDumpBuilder::Ptr CreateRawDumpBuilder()
  {
    return MakePtr<RawDumpBuilder>();
  }

  Dumper::Ptr CreateRawStreamDumper(const DumperParameters& params)
  {
    auto builder = CreateRawDumpBuilder();
    return CreateDumper(params, std::move(builder));
  }
}  // namespace Devices::AYM
