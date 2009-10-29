/*
Abstract:
  Containers plugins list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED__
#define __CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED__

namespace ZXTune
{
  //forward declaration
  class PluginsEnumerator;
  
  void RegisterRawContainer(PluginsEnumerator& enumerator);
  
  void RegisterContainerPlugins(PluginsEnumerator& enumerator)
  {
    RegisterRawContainer(enumerator);
  }
}

#endif //__CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED__
