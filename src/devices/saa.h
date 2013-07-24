/*
Abstract:
  SAA1099 chip interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_SAA_H_DEFINED
#define DEVICES_SAA_H_DEFINED

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
  namespace SAA
  {
    //6 tones + 2 noises + 2 envelopes
    const uint_t VOICES = 10;

    typedef Time::Microseconds Stamp;

    struct DataChunk
    {
      //registers offsets in data
      enum
      {
        REG_LEVEL0 = 0,
        REG_LEVEL1,
        REG_LEVEL2,
        REG_LEVEL3,
        REG_LEVEL4,
        REG_LEVEL5,

        REG_TONENUMBER0 = 8,
        REG_TONENUMBER1,
        REG_TONENUMBER2,
        REG_TONENUMBER3,
        REG_TONENUMBER4,
        REG_TONENUMBER5,

        REG_TONEOCTAVE01 = 16,
        REG_TONEOCTAVE23,
        REG_TONEOCTAVE45,

        REG_TONEMIXER = 20,
        REG_NOISEMIXER = 21,
        REG_NOISECLOCK = 22,

        REG_ENVELOPE0 = 24,
        REG_ENVELOPE1,

        REG_LAST = 32
      };

      DataChunk() : TimeStamp(), Mask(), Data()
      {
      }

      Stamp TimeStamp;
      uint32_t Mask;
      boost::array<uint8_t, REG_LAST> Data;
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

    typedef FixedChannelsCountState<VOICES> ChannelsState;

    // Describes real device
    class Chip : public Device
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;

      virtual void GetState(ChannelsState& state) const = 0;
    };

    enum InterpolationType
    {
      INTERPOLATION_NONE = 0,
      INTERPOLATION_LQ = 1,
      INTERPOLATION_HQ = 2
    };

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;

      virtual ~ChipParameters() {}

      virtual uint_t Version() const = 0;
      virtual uint64_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
      virtual InterpolationType Interpolation() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
  }
}

#endif //DEVICES_SAA_H_DEFINED
