/*
Abstract:
  AY/YM chips interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of UnrealSpeccy by SMT and Xpeccy sources by SamStyle
*/

//local includes
#include "psg.h"
#include "soundchip.h"
//boost includes
#include <boost/make_shared.hpp>

namespace Devices
{
namespace AYM
{
  BOOST_STATIC_ASSERT(Registers::TOTAL == 14);
  BOOST_STATIC_ASSERT(sizeof(Registers) == 16);

  struct Traits
  {
    typedef DataChunk DataChunkType;
    typedef PSG PSGType;
    typedef Chip ChipBaseType;
    static const uint_t VOICES = AYM::VOICES;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, MixerType::Ptr mixer, Sound::Receiver::Ptr target)
  {
    return boost::make_shared<SoundChip<Traits> >(params, mixer, target);
  }
}
}
