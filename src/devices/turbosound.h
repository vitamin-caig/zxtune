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
      virtual void Flush() = 0;
      virtual void Reset() = 0;
    };

    class Chip : public Device, public StateSource
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;
    };

    using AYM::ChipParameters;

    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target);
  }
}

#endif //DEVICES_TURBOSOUND_DEFINED
