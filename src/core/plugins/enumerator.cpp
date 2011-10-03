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
 
  void RegisterAllPlugins(PluginsRegistrator& registrator)
  {
    RegisterContainerPlugins(registrator);
    RegisterArchivePlugins(registrator);
    RegisterPlayerPlugins(registrator);
  }

  class PluginsContainer : public PluginsRegistrator
                         , public PluginsEnumerator
  {
  public:
    PluginsContainer()
    {
      RegisterAllPlugins(*this);
    }

    virtual void RegisterPlugin(PlayerPlugin::Ptr plugin)
    {
      const Plugin::Ptr description = plugin->GetDescription();
      AllPlugins.push_back(description);
      PlayerPlugins.push_back(plugin);
      Log::Debug(THIS_MODULE, "Registered player %1%", description->Id());
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
  {
  public:
    DetectCallbackAdapter(const DetectParameters& detectParams, Parameters::Accessor::Ptr coreParams)
      : DetectParams(detectParams)
      , CoreParams(coreParams)
    {
    }

    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      return CoreParams;
    }

    virtual Parameters::Accessor::Ptr CreateModuleParameters(DataLocation::Ptr location) const
    {
      const DataPath::Ptr subPath = location->GetPath();
      return DetectParams.CreateModuleParams(subPath->AsString());
    }

    virtual void ProcessModule(DataLocation::Ptr location, Module::Holder::Ptr holder) const
    {
      const DataPath::Ptr subPath = location->GetPath();
      return DetectParams.ProcessModule(subPath->AsString(), holder);
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return DetectParams.GetProgressCallback();
    }
  private:
    const DetectParameters& DetectParams;
    const Parameters::Accessor::Ptr CoreParams;
  };

  class SimplePluginDescription : public Plugin
  {
  public:
    SimplePluginDescription(const String& id, const String& info, uint_t capabilities)
      : ID(id)
      , Info(info)
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

    virtual uint_t Capabilities() const
    {
      return Caps;
    }
  private:
    const String ID;
    const String Info;
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

  Plugin::Iterator::Ptr EnumeratePlugins()
  {
    //TODO: remove
    return PluginsEnumerator::Create()->Enumerate();
  }

  Plugin::Ptr CreatePluginDescription(const String& id, const String& info, uint_t capabilities)
  {
    return boost::make_shared<SimplePluginDescription>(id, info, capabilities);
  }

  Error DetectModules(Parameters::Accessor::Ptr moduleParams, const DetectParameters& detectParams,
    Binary::Container::Ptr data, const String& startSubpath)
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
      return e;
    }
    catch (const std::bad_alloc&)
    {
      return Error(THIS_LINE, Module::ERROR_NO_MEMORY, Text::MODULE_ERROR_NO_MEMORY);
    }
  }

  Error OpenModule(Parameters::Accessor::Ptr moduleParams, Binary::Container::Ptr data, const String& subpath,
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
