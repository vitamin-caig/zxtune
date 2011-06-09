/*
Abstract:
  FYM dumper imlementation

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

  void StoreString(Dump& res, const String& str)
  {
    std::copy(str.begin(), str.end(), std::back_inserter(res));
    res.push_back(0);
  }

  class FYMBuilder : public FramedDumpBuilder
  {
  public:
    FYMBuilder(uint_t clocksPerFrame, uint64_t clockFreq, const String& title, const String& author, uint_t loopFrame)
      : ClocksPerFrame(clocksPerFrame)
      , ClockFreq(clockFreq)
      , Title(title)
      , Author(author)
      , LoopFrame(loopFrame)
      , Delegate(CreateRawDumpBuilder())
      , LastTick(1)
    {
    }

    virtual void Initialize()
    {
      LastTick = 1;
      return Delegate->Initialize();
    }

    virtual void GetResult(Dump& data) const
    {
      Dump rawDump;
      Delegate->GetResult(rawDump);

      assert(0 == LastTick % ClocksPerFrame);
      //calculate frames count by timing due to mutable size
      const uint_t framesCount = static_cast<uint_t>(LastTick / ClocksPerFrame);
      const std::size_t headerSize = sizeof(FYMHeader) + (Title.size() + 1) + (Author.size() + 1);

      Dump result(sizeof(FYMHeader));
      {
        FYMHeader* const header = safe_ptr_cast<FYMHeader*>(&result[0]);
        header->HeaderSize = static_cast<uint32_t>(headerSize);
        header->FramesCount = framesCount;
        header->LoopFrame = LoopFrame;
        header->PSGFreq = static_cast<uint32_t>(ClockFreq);
        header->IntFreq = static_cast<uint32_t>(ClockFreq * framesCount / LastTick);
      }
      StoreString(result, Title);
      StoreString(result, Author);

      result.resize(headerSize + rawDump.size());

      assert(0 == rawDump.size() % DataChunk::REG_LAST);
      for (uint_t reg = 0; reg < DataChunk::REG_LAST; ++reg)
      {
        for (uint_t frm = 0; frm < framesCount; ++frm)
        {
          result[headerSize + framesCount * reg + frm] = rawDump[DataChunk::REG_LAST * frm + reg];
        }
      }
      data.swap(result);
    }

    virtual void WriteFrame(uint_t framesPassed, const DataChunk& state, const DataChunk& update)
    {
      LastTick = state.Tick;
      return Delegate->WriteFrame(framesPassed, state, update);
    }
  private:
    const uint_t ClocksPerFrame;
    const uint64_t ClockFreq;
    const String Title;
    const String Author;
    const uint_t LoopFrame;
    const FramedDumpBuilder::Ptr Delegate;
    uint64_t LastTick;
  };
}

namespace Devices
{
  namespace AYM
  {
    Dumper::Ptr CreateFYMDumper(uint_t clocksPerFrame, uint64_t clockFreq, const String& title, const String& author, uint_t loopFrame)
    {
      const FramedDumpBuilder::Ptr builder = boost::make_shared<FYMBuilder>(clocksPerFrame, clockFreq, title, author, loopFrame);
      return CreateDumper(clocksPerFrame, builder);
    }
  }
}
