/**
* 
* @file
*
* @brief  AY/YM dumper interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/aym.h>
#include <time/duration.h>

//supporting for AY/YM-based modules
namespace Devices
{
  namespace AYM
  {
    // Describes dump converter
    class Dumper : public Device
    {
    public:
      typedef std::shared_ptr<Dumper> Ptr;

      virtual void GetDump(Dump& result) const = 0;
    };

    class DumperParameters
    {
    public:
      typedef std::shared_ptr<const DumperParameters> Ptr;
      virtual ~DumperParameters() = default;

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
      typedef std::shared_ptr<const FYMDumperParameters> Ptr;

      virtual uint64_t ClockFreq() const = 0;
      virtual String Title() const = 0;
      virtual String Author() const = 0;
      virtual uint_t LoopFrame() const = 0;
    };

    Dumper::Ptr CreateFYMDumper(FYMDumperParameters::Ptr params);
  }
}
