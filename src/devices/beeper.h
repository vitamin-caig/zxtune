/**
* 
* @file
*
* @brief  Beeper support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <sound/receiver.h>
#include <time/instant.h>
//std includes
#include <memory>

namespace Devices
{
  namespace Beeper
  {
    using TimeUnit = Time::Microsecond;
    using Stamp = Time::Instant<TimeUnit>;

    struct DataChunk
    {
      DataChunk() : TimeStamp(), Level()
      {
      }

      Stamp TimeStamp;
      bool Level;
    };

    class Device
    {
    public:
      typedef std::shared_ptr<Device> Ptr;
      virtual ~Device() = default;

      /// Render multiple data chunks
      virtual void RenderData(const std::vector<DataChunk>& src) = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };
    
    //TODO: add StateSource if required
    class Chip : public Device
    {
    public:
      typedef std::shared_ptr<Chip> Ptr;
    };

    class ChipParameters
    {
    public:
      typedef std::shared_ptr<const ChipParameters> Ptr;

      virtual ~ChipParameters() = default;

      virtual uint_t Version() const = 0;
      virtual uint64_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
  }
}
