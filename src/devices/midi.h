/**
* 
* @file
*
* @brief  MIDI device support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <devices/state.h>
#include <sound/receiver.h>
#include <time/stamp.h>

namespace Devices
{
  namespace MIDI
  {
    typedef Time::Microseconds Stamp;

    struct DataChunk
    {
      DataChunk()
        : TimeStamp()
      {
      }

      Stamp TimeStamp;
      // Commands' sizes in Data array
      std::vector<size_t> Sizes;
      //f0 xx xx xx - sysex
      //xx xx xx - else (3 bytes mandatory)
      Dump Data;
    };
    
    class Device
    {
    public:
      typedef boost::shared_ptr<Device> Ptr;
      virtual ~Device() {}

      /// Render sound till src.TimeStamp and change state
      virtual void RenderData(const DataChunk& src) = 0;

      /// Same as RenderData but do not produce sound output
      virtual void UpdateState(const DataChunk& src) = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };
    
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
      virtual uint_t SoundFreq() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
  }
}
