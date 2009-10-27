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
    String Id;
    String Description;
    String Version;
    uint32_t Capabilities;
  };
    
  /// Enumeration
  void GetSupportedPlugins(std::vector<PluginInformation>& plugins);
}

#endif //__CORE_PLUGIN_H_DEFINED__
