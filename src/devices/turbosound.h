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

    using AYM::TimeUnit;
    using AYM::Stamp;

    typedef std::array<AYM::Registers, CHIPS> Registers;

    struct DataChunk
    {
      DataChunk()
        : TimeStamp()
        , Data()
      {
      }

      Stamp TimeStamp;
      Registers Data;
    };

    class Device
    {
    public:
      typedef std::shared_ptr<Device> Ptr;
      virtual ~Device() = default;

      virtual void RenderData(const DataChunk& src) = 0;
      virtual void RenderData(const std::vector<DataChunk>& src) = 0;
      virtual void Reset() = 0;
    };

    class Chip : public Device, public StateSource
    {
    public:
      typedef std::shared_ptr<Chip> Ptr;
    };

    using AYM::ChipParameters;
    using AYM::MixerType;

    Chip::Ptr CreateChip(ChipParameters::Ptr params, MixerType::Ptr mixer, Sound::Receiver::Ptr target);
  }
}
