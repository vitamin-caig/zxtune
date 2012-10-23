/*
Abstract:
  Plugins registrator interface and related

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_REGISTRATOR_H_DEFINED__
#define __CORE_PLUGINS_REGISTRATOR_H_DEFINED__

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

#endif //__CORE_PLUGINS_REGISTRATOR_H_DEFINED__
