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
#include <time/stamp.h>
//boost includes
#include <boost/array.hpp>

namespace Devices
{
  namespace FM
  {
    const uint_t CHANNELS = 1;
    const uint_t VOICES = 3;

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

      Time::Nanoseconds TimeStamp;
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

    //channels state
    struct ChanState
    {
      ChanState()
        : Name(' '), Enabled(), Band(), LevelInPercents()
      {
      }

      explicit ChanState(Char name)
        : Name(name), Enabled(), Band(), LevelInPercents()
      {
      }

      //Short channel abbreviation
      Char Name;
      //Is channel enabled to output
      bool Enabled;
      //Currently played tone band (up to 96)
      uint_t Band;
      //Currently played tone level percentage
      uint_t LevelInPercents;
    };
    typedef boost::array<ChanState, VOICES> ChannelsState;

    // Describes real device
    class Chip : public Device
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;

      virtual void GetState(ChannelsState& state) const = 0;
    };

    // Sound is rendered in unsigned 16-bit values
    typedef uint16_t Sample;
    // Result sound stream receiver
    typedef DataReceiver<Sample> Receiver;

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;

      virtual ~ChipParameters() {}

      virtual uint64_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Receiver::Ptr target);
  }
}

#endif //DEVICES_FM_H_DEFINED
