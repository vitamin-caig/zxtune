/**
* 
* @file
*
* @brief  Archive plugin factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "core/plugins/plugins_types.h"
//library includes
#include <formats/packed.h>

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(const String& id, uint_t caps, Formats::Packed::Decoder::Ptr decoder);
}
