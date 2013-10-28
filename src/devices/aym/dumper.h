/*
Abstract:
  AY/YM dumper interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_AYM_DUMPER_H_DEFINED
#define DEVICES_AYM_DUMPER_H_DEFINED

//library includes
#include <devices/aym.h>

//supporting for AY/YM-based modules
namespace Devices
{
  namespace AYM
  {
    // Describes dump converter
    class Dumper : public Device
    {
    public:
      typedef boost::shared_ptr<Dumper> Ptr;

      virtual void GetDump(Dump& result) const = 0;
    };

    class DumperParameters
    {
    public:
      typedef boost::shared_ptr<const DumperParameters> Ptr;
      virtual ~DumperParameters() {}

      enum Optimization
      {
        NONE,
        NORMAL,
      };

      virtual Time::Microseconds FrameDuration() const = 0;
      virtual Optimization OptimizationLevel() const = 0;
    };

    Dumper::Ptr CreatePSGDumper(DumperParameters::Ptr params);
    Dumper::Ptr CreateZX50Dumper(DumperParameters::Ptr params);
    Dumper::Ptr CreateDebugDumper(DumperParameters::Ptr params);
    Dumper::Ptr CreateRawStreamDumper(DumperParameters::Ptr params);

    class FYMDumperParameters : public DumperParameters
    {
    public:
      typedef boost::shared_ptr<const FYMDumperParameters> Ptr;

      virtual uint64_t ClockFreq() const = 0;
      virtual String Title() const = 0;
      virtual String Author() const = 0;
      virtual uint_t LoopFrame() const = 0;
    };

    Dumper::Ptr CreateFYMDumper(FYMDumperParameters::Ptr params);
  }
}

#endif //DEVICES_AYM_DUMPER_H_DEFINED
