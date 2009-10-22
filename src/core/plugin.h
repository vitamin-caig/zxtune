/*
Abstract:
  Plugins related interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_PLUGIN_H_DEFINED__
#define __CORE_PLUGIN_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  /// Basic plugin information
  struct PluginInformation
  {
    PluginInformation() : Capabilities()
    {
    }
    uint32_t Capabilities;
    ParametersMap Properties;
  };
    
  /// Enumeration
  void GetSupportedPlugins(std::vector<PluginInformation>& plugins);
}

#endif //__CORE_PLUGIN_H_DEFINED__
