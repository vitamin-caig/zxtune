/**
* 
* @file
*
* @brief  Archive plugins lite factory
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
    RegisterDecompilePlugins(registrator);
  }
}
