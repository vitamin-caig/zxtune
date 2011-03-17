/*
Abstract:
  Module container and related interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CONTAINER_H_DEFINED__
#define __CORE_PLUGINS_CONTAINER_H_DEFINED__

//local includes
#include "plugins_chain.h"
//library includes
#include <io/container.h>

namespace ZXTune
{
  namespace Module
  {
    // Describes piece of data and defenitely location inside
    class Container
    {
    public:
      typedef boost::shared_ptr<const Container> Ptr;
      virtual ~Container() {}

      virtual IO::DataContainer::Ptr GetData() const = 0;
      virtual String GetPath() const = 0;
      virtual PluginsChain::ConstPtr GetPlugins() const = 0;
    };

    Container::Ptr CreateSubcontainer(Container::Ptr parent, Plugin::Ptr subPlugin, IO::DataContainer::Ptr subData, const String& subPath);
  }

  //module container descriptor- all required data
  //TODO: remove
  struct MetaContainer
  {
    MetaContainer()
      : Plugins(PluginsChain::Create())
    {
    }

    MetaContainer(const MetaContainer& rh)
      : Data(rh.Data)
      , Path(rh.Path)
      , Plugins(rh.Plugins->Clone())
    {
    }

    IO::DataContainer::Ptr Data;
    String Path;
    PluginsChain::Ptr Plugins;
  };

  inline MetaContainer MetaContainerFromContainer(const Module::Container& container)
  {
    MetaContainer result;
    result.Data = container.GetData();
    result.Path = container.GetPath();
    result.Plugins = container.GetPlugins()->Clone();
    return result;
  }
}

#endif //__CORE_PLUGINS_CONTAINER_H_DEFINED__
