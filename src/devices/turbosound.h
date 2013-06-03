/*
Abstract:
  TurboSound chip adapter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_TURBOSOUND_DEFINED
#define DEVICES_TURBOSOUND_DEFINED

//library includes
#include <devices/aym/chip.h>

namespace Devices
{
  namespace TurboSound
  {
    const uint_t CHIPS = 2;
    const uint_t VOICES = Devices::AYM::VOICES * CHIPS;

    struct DataChunk
    {
      DataChunk() : TimeStamp()
      {
      }

      AYM::Stamp TimeStamp;
      //TODO
      uint32_t Mask0;
      boost::array<uint8_t, AYM::DataChunk::REG_LAST> Data0;
      uint32_t Mask1;
      boost::array<uint8_t, AYM::DataChunk::REG_LAST> Data1;
    };

    class Device
    {
    public:
      typedef boost::shared_ptr<Device> Ptr;
      virtual ~Device() {}

      virtual void RenderData(const DataChunk& src) = 0;
      virtual void Flush() = 0;
      virtual void Reset() = 0;
    };

    using Devices::AYM::ChanState;
    typedef boost::array<ChanState, VOICES> ChannelsState;

    class Chip : public Device
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;

      virtual void GetState(ChannelsState& state) const = 0;
    };

    using Devices::AYM::ChipParameters;
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
    std::pair<AYM::Chip::Ptr, AYM::Chip::Ptr> CreateChipsPair(ChipParameters::Ptr params, Sound::Receiver::Ptr target);
  }
}

#endif //DEVICES_TURBOSOUND_DEFINED
