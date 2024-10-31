/**
 *
 * @file
 *
 * @brief  ROMs access interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/sid/roms.h"

namespace Module::Sid
{
  const uint8_t KERNAL[] = {
#include "core/plugins/players/sid/kernal.inc"
  };

  const uint8_t BASIC[] = {
#include "core/plugins/players/sid/basic.inc"
  };

  const uint8_t CHARGEN[] = {
#include "core/plugins/players/sid/chargen.inc"
  };

  const uint8_t* GetKernalROM()
  {
    return KERNAL;
  }

  const uint8_t* GetBasicROM()
  {
    return BASIC;
  }

  const uint8_t* GetChargenROM()
  {
    return CHARGEN;
  }
}  // namespace Module::Sid
