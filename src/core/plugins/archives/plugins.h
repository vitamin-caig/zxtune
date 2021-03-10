/**
 *
 * @file
 *
 * @brief  Different single-filed archive plugins factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include <core/plugins/archive_plugins_registrator.h>

namespace ZXTune
{
  void RegisterRawContainer(ArchivePluginsRegistrator& registrator);
  void RegisterMultitrackContainers(ArchivePluginsRegistrator& registrator);
  void RegisterZdataContainer(ArchivePluginsRegistrator& registrator);
  void RegisterArchiveContainers(ArchivePluginsRegistrator& registrator);
  void RegisterZXArchiveContainers(ArchivePluginsRegistrator& registrator);
  void RegisterDepackPlugins(ArchivePluginsRegistrator& registrator);
  void RegisterChiptunePackerPlugins(ArchivePluginsRegistrator& registrator);
  void RegisterDecompilePlugins(ArchivePluginsRegistrator& registrator);
}  // namespace ZXTune
