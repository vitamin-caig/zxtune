/*
Abstract:
  FM chips interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_FM_H_DEFINED
#define DEVICES_FM_H_DEFINED

//common includes
#include <data_streaming.h>
#include <types.h>
//library includes
#include <devices/state.h>
#include <sound/receiver.h>
#include <time/stamp.h>
//boost includes
#include <boost/array.hpp>

namespace Devices
{
  namespace FM
  {
    const uint_t VOICES = 3;

    typedef Time::Microseconds Stamp;

    struct DataChunk
    {
      DataChunk() : TimeStamp()
      {
      }

      struct Register
      {
        uint8_t Index;
        uint8_t Value;

        Register()
          : Index()
          , Value()
        {
        }

        Register(uint8_t idx, uint8_t val)
          : Index(idx)
          , Value(val)
        {
        }
      };
      typedef std::vector<Register> Registers;

      Stamp TimeStamp;
      Registers Data;
    };

    class Device
    {
    public:
      typedef boost::shared_ptr<Device> Ptr;
      virtual ~Device() {}

      /// render single data chunk
      virtual void RenderData(const DataChunk& src) = 0;

      /// flush any collected data
      virtual void Flush() = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    // Describes real device
    class Chip : public Device, public StateSource
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
    };

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;

      virtual ~ChipParameters() {}

      virtual uint_t Version() const = 0;
      virtual uint64_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
  }
}

#endif //DEVICES_FM_H_DEFINED
