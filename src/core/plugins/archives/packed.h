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

// local includes
#include "core/plugins/archive_plugin.h"
// library includes
#include <formats/packed.h>

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(PluginId id, uint_t caps, Formats::Packed::Decoder::Ptr decoder);
}
