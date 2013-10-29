/**
* 
* @file
*
* @brief  TurboSound support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/aym/chip.h>

namespace Devices
{
  namespace TurboSound
  {
    const uint_t CHIPS = 2;
    const uint_t VOICES = AYM::VOICES * CHIPS;

    using AYM::Stamp;

    typedef boost::array<AYM::Registers, CHIPS> Registers;

    struct DataChunk
    {
      DataChunk() : TimeStamp()
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

      virtual void RenderData(const DataChunk& src) = 0;
      virtual void Reset() = 0;
    };

    class Chip : public Device, public StateSource
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
    };

    using AYM::ChipParameters;
    using AYM::MixerType;

    Chip::Ptr CreateChip(ChipParameters::Ptr params, MixerType::Ptr mixer, Sound::Receiver::Ptr target);
  }
}
