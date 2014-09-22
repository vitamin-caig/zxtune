/**
* 
* @file
*
* @brief  Container plugins full factory
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include <core/plugins/containers/plugins.h>

namespace ZXTune
{
  void RegisterContainerPlugins(ArchivePluginsRegistrator& registrator)
  {
    //process raw container first
    RegisterRawContainer(registrator);

    RegisterArchiveContainers(registrator);
    //process containers last
    RegisterAyContainer(registrator);
    RegisterSidContainer(registrator);
    RegisterZdataContainer(registrator);
  }
}
