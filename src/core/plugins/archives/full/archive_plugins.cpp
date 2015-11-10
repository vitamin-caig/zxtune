/**
* 
* @file
*
* @brief  Archive plugins full factory
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include <core/plugins/archives/plugins.h>

namespace ZXTune
{
  void RegisterArchivePlugins(ArchivePluginsRegistrator& registrator)
  {
    RegisterDepackPlugins(registrator);
    RegisterChiptunePackerPlugins(registrator);
    RegisterDecompilePlugins(registrator);
  }
}
