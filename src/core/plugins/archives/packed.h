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

#include "core/plugins/archive_plugin.h"

#include "formats/packed.h"

#include "types.h"

namespace ZXTune
{
  class PluginId;

  ArchivePlugin::Ptr CreateArchivePlugin(PluginId id, uint_t caps, Formats::Packed::Decoder::Ptr decoder);
}  // namespace ZXTune
