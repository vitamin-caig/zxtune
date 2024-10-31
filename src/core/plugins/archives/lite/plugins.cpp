/**
 *
 * @file
 *
 * @brief  Archive plugins lite factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <core/plugins/archives/plugins.h>

namespace ZXTune
{
  void RegisterArchivePlugins(ArchivePluginsRegistrator& registrator)
  {
    RegisterMultitrackContainers(registrator);
    RegisterArchiveContainers(registrator);
    RegisterChiptunePackerPlugins(registrator);
    RegisterDecompilePlugins(registrator);
  }
}  // namespace ZXTune
