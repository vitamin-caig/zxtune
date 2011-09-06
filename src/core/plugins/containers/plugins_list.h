/*
Abstract:
  Containers plugins list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED__
#define __CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED__

namespace ZXTune
{
  //forward declaration
  class PluginsRegistrator;

  void RegisterRawContainer(PluginsRegistrator& registrator);
  void RegisterTRDContainer(PluginsRegistrator& registrator);
  void RegisterSCLContainer(PluginsRegistrator& registrator);
  void RegisterHRIPContainer(PluginsRegistrator& registrator);
  void RegisterZXZipContainer(PluginsRegistrator& registrator);
  void RegisterZipContainer(PluginsRegistrator& registrator);
  void RegisterRarContainer(PluginsRegistrator& registrator);

  void RegisterContainerPlugins(PluginsRegistrator& registrator)
  {
    //process raw container first
    RegisterRawContainer(registrator);
    RegisterTRDContainer(registrator);
    RegisterSCLContainer(registrator);
    RegisterHRIPContainer(registrator);
    RegisterZXZipContainer(registrator);
    RegisterZipContainer(registrator);
    RegisterRarContainer(registrator);
  }
}

#endif //__CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED__
