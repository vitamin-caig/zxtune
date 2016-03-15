/**
* 
* @file
*
* @brief  ROMs access interface
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "roms.h"

namespace Module
{
namespace Sid
{
  const uint8_t KERNAL[] =
  {
#include "kernal.inc"
  };

  const uint8_t BASIC[] =
  {
#include "basic.inc"
  };

  const uint8_t CHARGEN[] =
  {
#include "chargen.inc"
  };
}
}

namespace Module
{
namespace Sid
{
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
}
}
