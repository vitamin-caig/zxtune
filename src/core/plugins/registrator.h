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

namespace ZXTune
{
  template<class PluginType>
  class PluginsRegistrator
  {
  public:
    virtual ~PluginsRegistrator() {}

    virtual void RegisterPlugin(typename PluginType::Ptr plugin) = 0;
  };
}
