/*
Abstract:
  Plugins enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "registrator.h"
#include "archives/plugins_list.h"
#include "containers/plugins_list.h"
#include "players/plugins_list.h"
#include "core/src/callback.h"
#include "core/src/core.h"
//common includes
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/module_detect.h>
//std includes
#include <list>
//boost includes
#include <boost/make_shared.hpp>
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
      if (Filter.IsPluginEnabled(*plugin->GetDescription()))
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
      const Plugin::Ptr description = plugin->GetDescription();
      AllPlugins.push_back(description);
      ArchivePlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered archive container %1%", description->Id());
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
  private:
    PluginsArray AllPlugins;
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

    virtual PluginsEnumerator::Ptr GetUsedPlugins() const
    {
      return Plugins;
    }

    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      return CoreParams;
    }

    virtual Parameters::Accessor::Ptr CreateModuleParameters(const DataLocation& location) const
    {
      const DataPath::Ptr subPath = location.GetPath();
      return DetectParams.CreateModuleParams(subPath->AsString());
    }

    virtual Error ProcessModule(const DataLocation& location, Module::Holder::Ptr holder) const
    {
      const DataPath::Ptr subPath = location.GetPath();
      return DetectParams.ProcessModule(subPath->AsString(), holder);
    }

    virtual Log::ProgressCallback* GetProgress() const
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

  class SimplePluginDescription : public Plugin
  {
  public:
    SimplePluginDescription(const String& id, const String& info, const String& version, uint_t capabilities)
      : ID(id)
      , Info(info)
      , Vers(version)
      , Caps(capabilities)
    {
    }

    virtual String Id() const
    {
      return ID;
    }

    virtual String Description() const
    {
      return Info;
    }

    virtual String Version() const
    {
      return Vers;
    }

    virtual uint_t Capabilities() const
    {
      return Caps;
    }
  private:
    const String ID;
    const String Info;
    const String Vers;
    const uint_t Caps;
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

  Plugin::Iterator::Ptr EnumeratePlugins()
  {
    //TODO: remove
    return PluginsEnumerator::Create()->Enumerate();
  }

  Plugin::Ptr CreatePluginDescription(const String& id, const String& info, const String& version, uint_t capabilities)
  {
    return boost::make_shared<SimplePluginDescription>(id, info, version, capabilities);
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
      if (const DataLocation::Ptr location = OpenLocation(moduleParams, data, startSubpath))
      {
        const DetectCallbackAdapter callback(detectParams, moduleParams);
        Module::Detect(location, callback);
        return Error();
      }
      return MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, startSubpath);
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
      if (const DataLocation::Ptr location = OpenLocation(moduleParams, data, subpath))
      {
        const PluginsEnumerator::Ptr plugins = PluginsEnumerator::Create();
        if (Module::Holder::Ptr holder = Module::Open(location, plugins, moduleParams))
        {
          result = holder;
          return Error();
        }
      }
      return MakeFormattedError(THIS_LINE, Module::ERROR_FIND_SUBMODULE, Text::MODULE_ERROR_FIND_SUBMODULE, subpath);
    }
    catch (const std::bad_alloc&)
    {
      return Error(THIS_LINE, Module::ERROR_NO_MEMORY, Text::MODULE_ERROR_NO_MEMORY);
    }
  }
}
