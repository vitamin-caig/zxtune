/**
 *
 * @file
 *
 * @brief  Container plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "core/plugins/archive_plugin.h"
// library includes
#include <formats/archived.h>

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(PluginId id, uint_t caps, Formats::Archived::Decoder::Ptr decoder);

  String ProgressMessage(PluginId id, const String& path);
  String ProgressMessage(PluginId id, const String& path, const String& element);
}  // namespace ZXTune
