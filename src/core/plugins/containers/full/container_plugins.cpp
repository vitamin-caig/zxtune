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
    RegisterZXArchiveContainers(registrator);
    //process containers last
    RegisterMultitrackContainers(registrator);
    RegisterZdataContainer(registrator);
  }
}
