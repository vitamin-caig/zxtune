/**
 *
 * @file
 *
 * @brief  AY/YM-based chip implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "devices/aym/src/psg.h"
#include "devices/aym/src/soundchip.h"

#include "make_ptr.h"

#include <utility>

namespace Devices::AYM
{
  static_assert(Registers::TOTAL == 14, "Invalid registers count");
  static_assert(sizeof(Registers) == 16, "Invalid layout");

  struct Traits
  {
    using DataChunkType = DataChunk;
    using PSGType = PSG;
    using ChipBaseType = Chip;
    static const uint_t VOICES = AYM::VOICES;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, MixerType::Ptr mixer)
  {
    return MakePtr<SoundChip<Traits> >(std::move(params), std::move(mixer));
  }
}  // namespace Devices::AYM
