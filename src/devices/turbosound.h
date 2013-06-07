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
    std::pair<AYM::Chip::Ptr, AYM::Chip::Ptr> CreateChipsPair(AYM::ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target);
  }
}

#endif //DEVICES_TURBOSOUND_DEFINED
