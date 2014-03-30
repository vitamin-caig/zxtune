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

//local includes
#include <core/plugins/registrator.h>

namespace ZXTune
{
  void RegisterDepackPlugins(ArchivePluginsRegistrator& registrator);
  void RegisterDecompilePlugins(ArchivePluginsRegistrator& registrator);
}
