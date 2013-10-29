/**
* 
* @file
*
* @brief  AY/YM dump builder interface and factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/aym/dumper.h>

namespace Devices
{
  namespace AYM
  {
    class DumpBuilder
    {
    public:
      virtual ~DumpBuilder() {}

      virtual void Initialize() = 0;

      virtual void GetResult(Dump& result) const = 0;
    };

    class FramedDumpBuilder : public DumpBuilder
    {
    public:
      typedef boost::shared_ptr<FramedDumpBuilder> Ptr;

      virtual void WriteFrame(uint_t framesPassed, const Registers& state, const Registers& update) = 0;
    };

    Dumper::Ptr CreateDumper(DumperParameters::Ptr params, FramedDumpBuilder::Ptr builder);

    //internal factories
    FramedDumpBuilder::Ptr CreateRawDumpBuilder();
  }
}
