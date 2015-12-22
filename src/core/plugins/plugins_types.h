/**
* 
* @file
*
* @brief  Plugins types definitions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/plugin.h>

namespace ZXTune
{
  Plugin::Ptr CreatePluginDescription(const String& id, const String& info, uint_t capabilities);
}
