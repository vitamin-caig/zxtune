/**
* 
* @file
*
* @brief  AY/YM-based chip implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "psg.h"
#include "soundchip.h"
//common includes
#include <make_ptr.h>

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
    return MakePtr<SoundChip<Traits> >(params, mixer, target);
  }
}
}
