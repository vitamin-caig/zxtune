/*
Abstract:
  Plugins enumerator interface and related

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_ENUMERATOR_H_DEFINED__
#define __CORE_PLUGINS_ENUMERATOR_H_DEFINED__

//local includes
#include "plugins_types.h"
#include "registrator.h"

namespace ZXTune
{
  class PluginsEnumerator
  {
  public:
    typedef boost::shared_ptr<const PluginsEnumerator> Ptr;
    virtual ~PluginsEnumerator() {}

    virtual Plugin::Iterator::Ptr Enumerate() const = 0;
    virtual PlayerPlugin::Iterator::Ptr EnumeratePlayers() const = 0;
    virtual ArchivePlugin::Iterator::Ptr EnumerateArchives() const = 0;
    virtual ContainerPlugin::Iterator::Ptr EnumerateContainers() const = 0;

    //! Enumerate all supported plugins
    static Ptr Create();

    class Filter
    {
    public:
      virtual ~Filter() {}

      virtual bool IsPluginEnabled(const Plugin& plugin) const = 0;
    };
    //enumerate only plugins supported by filter
    static Ptr Create(const Filter& filter);
  };

  //TODO: compatibility
  class DetectParameters;

  //TODO: remove
  struct ModuleRegion
  {
    ModuleRegion() : Offset(), Size()
    {
    }
    ModuleRegion(std::size_t off, std::size_t sz)
      : Offset(off), Size(sz)
    {
    }
    std::size_t Offset;
    std::size_t Size;

    uint_t Checksum(const IO::DataContainer& container) const;
    IO::DataContainer::Ptr Extract(const IO::DataContainer& container) const;
  };

  class PluginsEnumeratorOld
  {
  public:
    //resolve subpath
    virtual void ResolveSubpath(const Parameters::Accessor& coreParams, IO::DataContainer::Ptr data,
      const String& subpath, MetaContainer& result) const = 0;
    //full module detection
    // @return Size of used data
    virtual std::size_t DetectModules(Parameters::Accessor::Ptr modulesParams, const DetectParameters& params, const MetaContainer& data) const = 0;
    //single module opening
    virtual Module::Holder::Ptr OpenModule(Parameters::Accessor::Ptr modulesParams, const MetaContainer& data) const = 0;

    //instantiator
    static PluginsEnumeratorOld& Instance();
  };
}

#endif //__CORE_PLUGINS_ENUMERATOR_H_DEFINED__
