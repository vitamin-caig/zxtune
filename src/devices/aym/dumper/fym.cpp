/**
 *
 * @file
 *
 * @brief  FYM dumper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "devices/aym/dumper/dump_builder.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/compression/zlib_stream.h>
#include <binary/data_builder.h>
// std includes
#include <utility>

namespace Devices::AYM
{
  struct FYMHeader
  {
    le_uint32_t HeaderSize;
    le_uint32_t FramesCount;
    le_uint32_t LoopFrame;
    le_uint32_t PSGFreq;
    le_uint32_t IntFreq;
  };

  static_assert(sizeof(FYMHeader) * alignof(FYMHeader) == 20, "Invalid layout");

  class FYMBuilder : public FramedDumpBuilder
  {
  public:
    explicit FYMBuilder(FYMDumperParameters::Ptr params)
      : Params(std::move(params))
      , Delegate(CreateRawDumpBuilder())
    {}

    void Initialize() override
    {
      return Delegate->Initialize();
    }

    Binary::Data::Ptr GetResult() override
    {
      Binary::DataBuilder output;
      {
        const auto unpacked = GetUnpackedResult();
        Binary::DataInputStream input(*unpacked);
        Binary::Compression::Zlib::Compress(input, output);
      }
      return output.CaptureResult();
    }

    void WriteFrame(uint_t framesPassed, const Registers& state, const Registers& update) override
    {
      return Delegate->WriteFrame(framesPassed, state, update);
    }

  private:
    Binary::Data::Ptr GetUnpackedResult() const
    {
      const auto rawDump = Delegate->GetResult();
      Require(0 == rawDump->Size() % Registers::TOTAL);
      const uint32_t framesCount = rawDump->Size() / Registers::TOTAL;
      const uint_t storedRegisters = Registers::TOTAL;
      const auto* const input = static_cast<const uint8_t*>(rawDump->Start());

      const auto& title = Params->Title();
      const auto& author = Params->Author();
      const uint32_t headerSize = sizeof(FYMHeader) + (title.size() + 1) + (author.size() + 1);
      const std::size_t contentSize = framesCount * storedRegisters;

      Binary::DataBuilder builder(headerSize + contentSize);
      auto& header = builder.Add<FYMHeader>();
      header.HeaderSize = headerSize;
      header.FramesCount = framesCount;
      header.LoopFrame = Params->LoopFrame();
      header.PSGFreq = Params->ClockFreq();
      header.IntFreq = Params->FrameDuration().ToFrequency();
      builder.AddCString(title);
      builder.AddCString(author);

      for (uint_t reg = 0; reg < storedRegisters; ++reg)
      {
        auto* const result = static_cast<uint8_t*>(builder.Allocate(framesCount));
        for (uint_t frm = 0, inOffset = reg; frm < framesCount; ++frm, inOffset += Registers::TOTAL)
        {
          result[frm] = input[inOffset];
        }
      }
      return builder.CaptureResult();
    }

  private:
    const FYMDumperParameters::Ptr Params;
    const FramedDumpBuilder::Ptr Delegate;
  };

  Dumper::Ptr CreateFYMDumper(FYMDumperParameters::Ptr params)
  {
    auto builder = MakePtr<FYMBuilder>(params);
    return CreateDumper(*params, std::move(builder));
  }
}  // namespace Devices::AYM
