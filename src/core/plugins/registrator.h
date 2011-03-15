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
  class PluginsRegistrator
  {
  public:
    virtual ~PluginsRegistrator() {}

    //endpoint modules support
    virtual void RegisterPlugin(PlayerPlugin::Ptr plugin) = 0;
    //archives containers support
    virtual void RegisterPlugin(ArchivePlugin::Ptr plugin) = 0;
    //nested containers support
    virtual void RegisterPlugin(ContainerPlugin::Ptr plugin) = 0;
  };
}

#endif //__CORE_PLUGINS_REGISTRATOR_H_DEFINED__
