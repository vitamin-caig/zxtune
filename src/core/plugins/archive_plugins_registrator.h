/**
 *
 * @file
 *
 * @brief  Archive plugins registrator interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "core/plugins/archive_plugin.h"
#include "core/plugins/registrator.h"

namespace ZXTune
{
  class ArchivePluginsRegistrator : public PluginsRegistrator<ArchivePlugin>
  {};
}  // namespace ZXTune
