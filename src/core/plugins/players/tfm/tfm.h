/*
Abstract:
  TFM chip adapter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TFM_DEFINED
#define CORE_PLUGINS_PLAYERS_TFM_DEFINED

//library includes
#include <devices/fm.h>

namespace Devices
{
  namespace TFM
  {
    const uint_t CHIPS = 2;
    const uint_t VOICES = Devices::FM::VOICES * CHIPS;

    struct DataChunk
    {
      DataChunk() : TimeStamp()
      {
      }

      FM::Stamp TimeStamp;
      boost::array<Devices::FM::DataChunk::Registers, CHIPS> Data;
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

    using Devices::FM::ChanState;
    typedef boost::array<ChanState, VOICES> ChannelsState;

    class Chip : public Device
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;

      virtual void GetState(ChannelsState& state) const = 0;
    };

    using Devices::FM::Sample;
    using Devices::FM::Receiver;
    using Devices::FM::ChipParameters;
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Receiver::Ptr target);
  }
}

#endif //CORE_PLUGINS_PLAYERS_TFM_DEFINED
