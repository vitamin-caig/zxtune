/*
Abstract:
  Module detection in container

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "callback.h"
#include "core.h"
#include "core/plugins/plugins_types.h"
//common includes
#include <error.h>
//library includes
#include <io/fs_tools.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <src/core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return ARCHIVE_PLUGIN_PREFIX + pluginId;
  }

  class OpenModuleCallback : public Module::DetectCallback
  {
  public:
    OpenModuleCallback(PluginsEnumerator::Ptr usedPlugins, Parameters::Accessor::Ptr moduleParams)
      : Plugins(usedPlugins)
      , ModuleParams(moduleParams)
    {
    }

    virtual PluginsEnumerator::Ptr GetUsedPlugins() const
    {
      return Plugins;
    }

    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      assert(!"Should not be called");
      return Parameters::Accessor::Ptr();
    }

    virtual Parameters::Accessor::Ptr CreateModuleParameters(const DataLocation& /*location*/) const
    {
      return ModuleParams;
    }

    virtual Error ProcessModule(const DataLocation& /*location*/, Module::Holder::Ptr holder) const
    {
      Result = holder;
      return Error();
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return 0;
    }

    Module::Holder::Ptr GetResult() const
    {
      return Result;
    }
  private:
    const PluginsEnumerator::Ptr Plugins;
    const Parameters::Accessor::Ptr ModuleParams;
    mutable Module::Holder::Ptr Result;
  };
  
  class DetectionContext
  {
  public:
    DetectionContext(DataLocation::Ptr location, const Module::DetectCallback& callback)
      : Location(location)
      , Data(Location->GetData())
      , Callback(callback)
      , UsedPlugins(Callback.GetUsedPlugins())
      , Params(Callback.GetPluginsParameters())
    {
    }
    
    std::size_t DetectContainer() const
    {
      for (ContainerPlugin::Iterator::Ptr iter = UsedPlugins->EnumerateContainers(); iter->IsValid(); iter->Next())
      {
        const ContainerPlugin::Ptr plugin = iter->Get();
        if (!plugin->Check(*Data))
        {
          continue;
        }
        if (std::size_t usedSize = plugin->Process(Location, Callback))
        {
          return usedSize;
        }
      }
      return 0;
    }

    std::size_t DetectArchive() const
    {
      for (ArchivePlugin::Iterator::Ptr iter = UsedPlugins->EnumerateArchives(); iter->IsValid(); iter->Next())
      {
        const ArchivePlugin::Ptr plugin = iter->Get();
        if (!plugin->Check(*Data))
        {
          continue;
        }
        std::size_t usedSize = 0;
        if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(*Params, *Data, usedSize))
        {
          const String subpath = EncodeArchivePluginToPath(plugin->Id());
          const DataLocation::Ptr subLocation = CreateNestedLocation(Location, plugin, subdata, subpath);
          Module::Detect(subLocation, Callback);
          return usedSize;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return 0;
    }

    std::size_t DetectModule() const
    {
      for (PlayerPlugin::Iterator::Ptr plugins = UsedPlugins->EnumeratePlayers(); plugins->IsValid(); plugins->Next())
      {
        const PlayerPlugin::Ptr plugin = plugins->Get();
        if (!plugin->Check(*Data))
        {
          continue;
        }
        const Parameters::Accessor::Ptr moduleParams = Callback.CreateModuleParameters(*Location);
        std::size_t usedSize = 0;
        if (Module::Holder::Ptr module = plugin->CreateModule(moduleParams, Location, usedSize))
        {
          ThrowIfError(Callback.ProcessModule(*Location, module));
          return usedSize;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return 0;
    }
  private:
    const DataLocation::Ptr Location;
    const IO::DataContainer::Ptr Data;
    const Module::DetectCallback& Callback;
    const PluginsEnumerator::Ptr UsedPlugins;
    const Parameters::Accessor::Ptr Params;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Holder::Ptr Open(DataLocation::Ptr location, PluginsEnumerator::Ptr usedPlugins, Parameters::Accessor::Ptr moduleParams)
    {
      const OpenModuleCallback callback(usedPlugins, moduleParams);
      const DetectionContext ctx(location, callback);
      ctx.DetectModule();
      return callback.GetResult();
    }

    std::size_t Detect(DataLocation::Ptr location, const DetectCallback& callback)
    {
      const DetectionContext ctx(location, callback);
      if (std::size_t usedSize = ctx.DetectContainer())
      {
        return usedSize;
      }
      if (std::size_t usedSize = ctx.DetectArchive())
      {
        return usedSize;
      }
      return ctx.DetectModule();
    }
  }
}
