/**
 *
 * @file
 *
 * @brief  Archive plugins full factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archives/plugins.h"

namespace ZXTune
{
  void RegisterArchivePlugins(ArchivePluginsRegistrator& registrator)
  {
    // process raw container first
    RegisterRawContainer(registrator);

    RegisterArchiveContainers(registrator);
    RegisterZXArchiveContainers(registrator);
    // process containers last
    RegisterMultitrackContainers(registrator);
    RegisterZdataContainer(registrator);

    // packed
    RegisterDepackPlugins(registrator);
    RegisterChiptunePackerPlugins(registrator);
    RegisterDecompilePlugins(registrator);
  }
}  // namespace ZXTune
