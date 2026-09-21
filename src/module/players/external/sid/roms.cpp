/**
 *
 * @file
 *
 * @brief  ROMs access interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/external/sid/roms.h"

namespace Module::Sid
{
  const uint8_t KERNAL[] = {
#include "module/players/external/sid/kernal.inc"
  };

  const uint8_t BASIC[] = {
#include "module/players/external/sid/basic.inc"
  };

  const uint8_t CHARGEN[] = {
#include "module/players/external/sid/chargen.inc"
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
