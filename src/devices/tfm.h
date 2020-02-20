/**
* 
* @file
*
* @brief  TurboFM support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/fm.h>

namespace Devices
{
  namespace TFM
  {
    const uint_t CHIPS = 2;
    const uint_t VOICES = FM::VOICES * CHIPS;

    using Devices::FM::TimeUnit;
    using Devices::FM::Stamp;

    class Register : public FM::Register
    {
    public:
      Register()
        : FM::Register()
      {
      }

      Register(uint_t chip, FM::Register reg)
        : FM::Register(std::move(reg))
      {
        Val |= chip << 16;
      }

      Register(uint_t chip, uint_t idx, uint_t val)
        : FM::Register(idx, val)
      {
        Val |= chip << 16;
      }

      uint_t Chip() const
      {
        return (Val >> 16) & 0xff;
      }
    };

    typedef std::vector<Register> Registers;

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
      typedef std::shared_ptr<Device> Ptr;
      virtual ~Device() = default;

      virtual void RenderData(const DataChunk& src) = 0;
      virtual void Reset() = 0;
    };

    class Chip : public Device, public StateSource
    {
    public:
      typedef std::shared_ptr<Chip> Ptr;
    };

    using FM::ChipParameters;
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
  }
}
