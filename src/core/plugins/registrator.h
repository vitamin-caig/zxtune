/**
* 
* @file
*
* @brief  Plugins registrator interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "plugins_types.h"

namespace ZXTune
{
  template<class PluginType>
  class PluginsRegistrator
  {
  public:
    virtual ~PluginsRegistrator() {}

    virtual void RegisterPlugin(typename PluginType::Ptr plugin) = 0;
  };

  typedef PluginsRegistrator<ArchivePlugin> ArchivePluginsRegistrator;
  typedef PluginsRegistrator<PlayerPlugin> PlayerPluginsRegistrator;
}
