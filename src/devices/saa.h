/**
* 
* @file
*
* @brief  SAA1099 support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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

    struct Registers
    {
      //registers offsets in data
      enum
      {
        LEVEL0 = 0,
        LEVEL1,
        LEVEL2,
        LEVEL3,
        LEVEL4,
        LEVEL5,

        TONENUMBER0 = 8,
        TONENUMBER1,
        TONENUMBER2,
        TONENUMBER3,
        TONENUMBER4,
        TONENUMBER5,

        TONEOCTAVE01 = 16,
        TONEOCTAVE23,
        TONEOCTAVE45,

        TONEMIXER = 20,
        NOISEMIXER = 21,
        NOISECLOCK = 22,

        ENVELOPE0 = 24,
        ENVELOPE1,

        TOTAL = 28
      };

      Registers()
        : Mask()
      {
      }

      uint32_t Mask;
      boost::array<uint8_t, TOTAL> Data;
    };

    struct DataChunk
    {

      DataChunk()
        : TimeStamp(), Data()
      {
      }

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

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    // Describes real device
    class Chip : public Device, public StateSource
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
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
