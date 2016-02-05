/**
* 
* @file
*
* @brief  FYM dumper implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "dump_builder.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/compress.h>
#include <binary/data_builder.h>
//std includes
#include <algorithm>
#include <iterator>

namespace Devices
{
namespace AYM
{
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct FYMHeader
  {
    uint32_t HeaderSize;
    uint32_t FramesCount;
    uint32_t LoopFrame;
    uint32_t PSGFreq;
    uint32_t IntFreq;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  class FYMBuilder : public FramedDumpBuilder
  {
  public:
    explicit FYMBuilder(FYMDumperParameters::Ptr params)
      : Params(params)
      , Delegate(CreateRawDumpBuilder())
    {
    }

    virtual void Initialize()
    {
      return Delegate->Initialize();
    }

    virtual void GetResult(Dump& data) const
    {
      Dump rawDump;
      Delegate->GetResult(rawDump);
      Require(0 == rawDump.size() % Registers::TOTAL);
      const uint32_t framesCount = rawDump.size() / Registers::TOTAL;
      const uint_t storedRegisters = Registers::TOTAL;

      const String& title = Params->Title();
      const String author = Params->Author();
      const uint32_t headerSize = sizeof(FYMHeader) + (title.size() + 1) + (author.size() + 1);
      const std::size_t contentSize = framesCount * storedRegisters;

      Binary::DataBuilder builder(headerSize + contentSize);
      FYMHeader& header = builder.Add<FYMHeader>();
      header.HeaderSize = fromLE(headerSize);
      header.FramesCount = fromLE(framesCount);
      header.LoopFrame = fromLE(static_cast<uint32_t>(Params->LoopFrame()));
      header.PSGFreq = fromLE(static_cast<uint32_t>(Params->ClockFreq()));
      header.IntFreq = fromLE(static_cast<uint32_t>(Time::GetFrequencyForPeriod(Params->FrameDuration())));
      builder.AddCString(title);
      builder.AddCString(author);

      for (uint_t reg = 0; reg < storedRegisters; ++reg)
      {
        uint8_t* const result = static_cast<uint8_t*>(builder.Allocate(framesCount));
        for (uint_t frm = 0, inOffset = reg; frm < framesCount; ++frm, inOffset += Registers::TOTAL)
        {
          result[frm] = rawDump[inOffset];
        }
      }
      Dump result;
      builder.CaptureResult(result);
      Binary::Compression::Zlib::Compress(result, data);
    }

    virtual void WriteFrame(uint_t framesPassed, const Registers& state, const Registers& update)
    {
      return Delegate->WriteFrame(framesPassed, state, update);
    }
  private:
    const FYMDumperParameters::Ptr Params;
    const FramedDumpBuilder::Ptr Delegate;
  };

  Dumper::Ptr CreateFYMDumper(FYMDumperParameters::Ptr params)
  {
    const FramedDumpBuilder::Ptr builder = MakePtr<FYMBuilder>(params);
    return CreateDumper(params, builder);
  }
}
}
