/**
*
* @file
*
* @brief  VGM format support tools
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>

namespace Module
{
  namespace VGM
  {
    String DetectPlatform(const Dump& blob);
  }
}
