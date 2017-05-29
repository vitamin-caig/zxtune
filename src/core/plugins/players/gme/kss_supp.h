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

//common includes
#include <types.h>

namespace Module
{
  namespace KSS
  {
    String DetectPlatform(const Dump& blob);
  }
}
