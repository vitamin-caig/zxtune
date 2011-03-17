/*
Abstract:
  Plugins enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "core.h"
#include "registrator.h"
#include "archives/plugins_list.h"
#include "containers/plugins_list.h"
#include "players/plugins_list.h"
//common includes
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <io/fs_tools.h>
//std includes
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/bind/apply.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/join.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG 04EDD719

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::Enumerator");

  typedef std::vector<Plugin::Ptr> PluginsArray;
  typedef std::vector<PlayerPlugin::Ptr> PlayerPluginsArray;
  typedef std::vector<ArchivePlugin::Ptr> ArchivePluginsArray;
  typedef std::vector<ContainerPlugin::Ptr> ContainerPluginsArray;

  class PluginsChainImpl : public PluginsChain
  {
    typedef std::list<Plugin::Ptr> PluginsList;
  public:
    PluginsChainImpl()
      : NestingCache(-1)
    {
    }

    PluginsChainImpl(const PluginsList& list)
      : Container(list)
      , NestingCache(-1)
    {
    }

    virtual void Add(Plugin::Ptr plugin)
    {
      Container.push_back(plugin);
      NestingCache = -1;
    }

    virtual Plugin::Ptr GetLast() const
    {
      if (!Container.empty())
      {
        return Container.back();
      }
      return Plugin::Ptr();
    }

    virtual PluginsChain::Ptr Clone() const
    {
      return boost::make_shared<PluginsChainImpl>(Container);
    }

    virtual uint_t Count() const
    {
      return Container.size();
    }

    virtual String AsString() const
    {
      StringArray ids(Container.size());
      std::transform(Container.begin(), Container.end(),
        ids.begin(), boost::mem_fn(&Plugin::Id));
      return boost::algorithm::join(ids, String(Text::MODULE_CONTAINERS_DELIMITER));
    }

    virtual uint_t CalculateContainersNesting() const
    {
      if (-1 == NestingCache)
      {
        NestingCache = std::count_if(Container.begin(), Container.end(),
          &IsMultitrackPlugin);
      }
      return static_cast<uint_t>(NestingCache);
    }
  private:
    static bool IsMultitrackPlugin(const Plugin::Ptr plugin)
    {
      const uint_t caps = plugin->Capabilities();
      return CAP_STOR_MULTITRACK == (caps & CAP_STOR_MULTITRACK);
    }
  private:
    PluginsList Container;
    mutable int_t NestingCache;
  };

  class FilteredPluginsRegistrator : public PluginsRegistrator
  {
  public:
    FilteredPluginsRegistrator(PluginsRegistrator& delegate, const PluginsEnumerator::Filter& filter)
      : Delegate(delegate)
      , Filter(filter)
    {
    }

    virtual void RegisterPlugin(PlayerPlugin::Ptr plugin)
    {
      if (Filter.IsPluginEnabled(*plugin))
      {
        Delegate.RegisterPlugin(plugin);
      }
    }

    virtual void RegisterPlugin(ArchivePlugin::Ptr plugin)
    {
      if (Filter.IsPluginEnabled(*plugin))
      {
        Delegate.RegisterPlugin(plugin);
      }
    }

    virtual void RegisterPlugin(ContainerPlugin::Ptr plugin)
    {
      if (Filter.IsPluginEnabled(*plugin))
      {
        Delegate.RegisterPlugin(plugin);
      }
    }
  private:
    PluginsRegistrator& Delegate;
    const PluginsEnumerator::Filter& Filter;
  };

  void RegisterAllPlugins(PluginsRegistrator& registrator)
  {
    RegisterContainerPlugins(registrator);
    RegisterArchivePlugins(registrator);
    RegisterPlayerPlugins(registrator);
  }

  void RegisterPlugins(PluginsRegistrator& registrator, const PluginsEnumerator::Filter& filter)
  {
    FilteredPluginsRegistrator filtered(registrator, filter);
    RegisterAllPlugins(filtered);
  }

  class PluginsContainer : public PluginsRegistrator
                         , public PluginsEnumerator
  {
  public:
    PluginsContainer()
    {
      RegisterAllPlugins(*this);
    }

    explicit PluginsContainer(const PluginsEnumerator::Filter& filter)
    {
      RegisterPlugins(*this, filter);
    }

    virtual void RegisterPlugin(PlayerPlugin::Ptr plugin)
    {
      AllPlugins.push_back(plugin);
      PlayerPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered player %1%", plugin->Id());
    }

    virtual void RegisterPlugin(ArchivePlugin::Ptr plugin)
    {
      AllPlugins.push_back(plugin);
      ArchivePlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered archive container %1%", plugin->Id());
    }

    virtual void RegisterPlugin(ContainerPlugin::Ptr plugin)
    {
      AllPlugins.push_back(plugin);
      ContainerPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered container %1%", plugin->Id());
    }

    //public interface
    virtual Plugin::Iterator::Ptr Enumerate() const
    {
      return CreateRangedObjectIteratorAdapter(AllPlugins.begin(), AllPlugins.end());
    }

    //private interface
    virtual PlayerPlugin::Iterator::Ptr EnumeratePlayers() const
    {
      return CreateRangedObjectIteratorAdapter(PlayerPlugins.begin(), PlayerPlugins.end());
    }

    virtual ArchivePlugin::Iterator::Ptr EnumerateArchives() const
    {
      return CreateRangedObjectIteratorAdapter(ArchivePlugins.begin(), ArchivePlugins.end());
    }

    virtual ContainerPlugin::Iterator::Ptr EnumerateContainers() const
    {
      return CreateRangedObjectIteratorAdapter(ContainerPlugins.begin(), ContainerPlugins.end());
    }
  private:
    PluginsArray AllPlugins;
    ContainerPluginsArray ContainerPlugins;
    ArchivePluginsArray ArchivePlugins;
    PlayerPluginsArray PlayerPlugins;
  };

  class DetectCallbackAdapter : public Module::DetectCallback
                              , private PluginsEnumerator::Filter
  {
  public:
    DetectCallbackAdapter(const DetectParameters& detectParams, Parameters::Accessor::Ptr coreParams)
      : DetectParams(detectParams)
      , CoreParams(coreParams)
    {
      Plugins = PluginsEnumerator::Create(*this);
    }

    virtual PluginsEnumerator::Ptr GetEnabledPlugins() const
    {
      return Plugins;
    }

    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      return CoreParams;
    }

    virtual Parameters::Accessor::Ptr GetModuleParameters(const Module::Container& container) const
    {
      return CoreParams;
    }

    virtual Error ProcessModule(const Module::Container& container, Module::Holder::Ptr holder) const
    {
      return DetectParams.ProcessModule(container.GetPath(), holder);
    }

    virtual Log::ProgressCallback* GetProgressCallback() const
    {
      return DetectParams.GetProgressCallback();
    }

    virtual bool IsPluginEnabled(const Plugin& plugin) const
    {
      return !DetectParams.FilterPlugin(plugin);
    }
  private:
    const DetectParameters& DetectParams;
    const Parameters::Accessor::Ptr CoreParams;
    PluginsEnumerator::Ptr Plugins;
  };
}

namespace ZXTune
{
  PluginsEnumerator::Ptr PluginsEnumerator::Create()
  {
    static PluginsContainer instance;
    return PluginsEnumerator::Ptr(&instance, NullDeleter<PluginsEnumerator>());
  }

  PluginsEnumerator::Ptr PluginsEnumerator::Create(const PluginsEnumerator::Filter& filter)
  {
    return PluginsEnumerator::Ptr(new PluginsContainer(filter));
  }

  PluginsChain::Ptr PluginsChain::Create()
  {
    return boost::make_shared<PluginsChainImpl>();
  }

  Plugin::Iterator::Ptr EnumeratePlugins()
  {
    //TODO: remove
    return PluginsEnumerator::Create()->Enumerate();
  }

  Error DetectModules(Parameters::Accessor::Ptr moduleParams, const DetectParameters& detectParams,
    IO::DataContainer::Ptr data, const String& startSubpath)
  {
    if (!data.get())
    {
      return Error(THIS_LINE, Module::ERROR_INVALID_PARAMETERS, Text::MODULE_ERROR_PARAMETERS);
    }
    try
    {
      const Module::Container::Ptr container = Module::OpenContainer(moduleParams, data, startSubpath);
      const DetectCallbackAdapter callback(detectParams, moduleParams);
      Module::DetectModules(container, callback);
      return Error();
    }
    catch (const Error& e)
    {
      Error err(THIS_LINE, Module::ERROR_DETECT_CANCELED, Text::MODULE_ERROR_CANCELED);
      return err.AddSuberror(e);
    }
    catch (const std::bad_alloc&)
    {
      return Error(THIS_LINE, Module::ERROR_NO_MEMORY, Text::MODULE_ERROR_NO_MEMORY);
    }
  }

  Error OpenModule(Parameters::Accessor::Ptr moduleParams, IO::DataContainer::Ptr data, const String& subpath,
      Module::Holder::Ptr& result)
  {
    if (!data.get())
    {
      return Error(THIS_LINE, Module::ERROR_INVALID_PARAMETERS, Text::MODULE_ERROR_PARAMETERS);
    }
    try
    {
      const Module::Container::Ptr container = Module::OpenContainer(moduleParams, data, subpath);
      if (Module::Holder::Ptr holder = Module::OpenModule(container, moduleParams))
      {
        result = holder;
        return Error();
      }
      return MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, subpath);
    }
    catch (const std::bad_alloc&)
    {
      return Error(THIS_LINE, Module::ERROR_NO_MEMORY, Text::MODULE_ERROR_NO_MEMORY);
    }
  }
}
