/**
*
* @file
*
* @brief  KSS format support tools
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/view.h>

namespace Module
{
  namespace KSS
  {
    String DetectPlatform(Binary::View blob);
  }
}
