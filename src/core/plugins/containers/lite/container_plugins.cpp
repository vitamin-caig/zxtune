/**
* 
* @file
*
* @brief  Container plugins lite factory
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <core/plugins/containers/plugins.h>

namespace ZXTune
{
  void RegisterContainerPlugins(ArchivePluginsRegistrator& registrator)
  {
    RegisterAyContainer(registrator);
    RegisterSidContainer(registrator);
  }
}
