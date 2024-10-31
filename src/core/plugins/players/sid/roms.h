/**
 *
 * @file
 *
 * @brief  ROMs access interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <types.h>

namespace Module::Sid
{
  const uint8_t* GetKernalROM();
  const uint8_t* GetBasicROM();
  const uint8_t* GetChargenROM();
}  // namespace Module::Sid
