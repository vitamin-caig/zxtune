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
  ArchivePlugin::Ptr CreateArchivePlugin(StringView id, uint_t caps, Formats::Archived::Decoder::Ptr decoder);

  String ProgressMessage(const String& id, const String& path);
  String ProgressMessage(const String& id, const String& path, const String& element);
}  // namespace ZXTune
