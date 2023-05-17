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

// library includes
#include <binary/data.h>
#include <devices/aym.h>
#include <time/duration.h>

// supporting for AY/YM-based modules
namespace Devices::AYM
{
  // Describes dump converter
  class Dumper : public Device
  {
  public:
    using Ptr = std::shared_ptr<Dumper>;

    virtual Binary::Data::Ptr GetDump() = 0;
  };

  class DumperParameters
  {
  public:
    virtual ~DumperParameters() = default;

    enum Optimization
    {
      NONE,
      NORMAL,
    };

    virtual Time::Microseconds FrameDuration() const = 0;
    virtual Optimization OptimizationLevel() const = 0;
  };

  Dumper::Ptr CreatePSGDumper(const DumperParameters& params);
  Dumper::Ptr CreateZX50Dumper(const DumperParameters& params);
  Dumper::Ptr CreateDebugDumper(const DumperParameters& params);
  Dumper::Ptr CreateRawStreamDumper(const DumperParameters& params);

  class FYMDumperParameters : public DumperParameters
  {
  public:
    using Ptr = std::shared_ptr<const FYMDumperParameters>;

    virtual uint64_t ClockFreq() const = 0;
    virtual String Title() const = 0;
    virtual String Author() const = 0;
    virtual uint_t LoopFrame() const = 0;
  };

  Dumper::Ptr CreateFYMDumper(FYMDumperParameters::Ptr params);
}  // namespace Devices::AYM
