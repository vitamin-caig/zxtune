/*
Abstract:
  TurboSound chip adapter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <devices/turbosound.h>

namespace Devices
{
  namespace TurboSound
  {
    std::pair<AYM::Chip::Ptr, AYM::Chip::Ptr> CreateChipsPair(AYM::ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target)
    {
      const std::pair<Sound::Receiver::Ptr, Sound::Receiver::Ptr> targets = Sound::CreateReceiversPair(target);
      return std::make_pair(AYM::CreateChip(params, mixer, targets.first), AYM::CreateChip(params, mixer, targets.second));
    }
  }
}
