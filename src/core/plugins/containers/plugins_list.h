/*
Abstract:
  Containers plugins list

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED
#define CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED

namespace ZXTune
{
  //forward declaration
  class PluginsRegistrator;

  void RegisterRawContainer(PluginsRegistrator& registrator);

  void RegisterContainerPlugins(PluginsRegistrator& registrator);
}

#endif //CORE_PLUGINS_CONTAINERS_LIST_H_DEFINED
