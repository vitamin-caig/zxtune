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

//library includes
#include <binary/view.h>

namespace Module
{
  namespace VideoGameMusic
  {
    String DetectPlatform(Binary::View blob);
  }
}
