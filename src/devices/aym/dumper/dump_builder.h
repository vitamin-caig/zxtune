/*
Abstract:
  AY dump builder interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_AYM_DUMP_BUILDER_DEFINED
#define DEVICES_AYM_DUMP_BUILDER_DEFINED

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

      virtual void WriteFrame(uint_t framesPassed, const DataChunk& state, const DataChunk& update) = 0;
    };

    Dumper::Ptr CreateDumper(DumperParameters::Ptr params, FramedDumpBuilder::Ptr builder);

    //internal factories
    FramedDumpBuilder::Ptr CreateRawDumpBuilder();
  }
}

#endif //DEVICES_AYM_DUMP_BUILDER_DEFINED
