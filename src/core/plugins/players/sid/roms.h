/**
* 
* @file
*
* @brief  ROMs access interface
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <types.h>

namespace Module
{
namespace Sid
{
  const uint8_t* GetKernalROM();
  const uint8_t* GetBasicROM();
  const uint8_t* GetChargenROM();
}//namespace Sid
}//namespace Module
