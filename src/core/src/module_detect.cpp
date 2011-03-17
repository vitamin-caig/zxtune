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
#include "core\plugins\plugins_types.h"
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

  std::size_t DetectContainer(DataLocation::Ptr location, const Module::DetectCallback& callback)
  {
    const PluginsEnumerator::Ptr usedPlugins = callback.GetUsedPlugins();
    const IO::DataContainer::Ptr data = location->GetData();
    for (ContainerPlugin::Iterator::Ptr iter = usedPlugins->EnumerateContainers(); iter->IsValid(); iter->Next())
    {
      const ContainerPlugin::Ptr plugin = iter->Get();
      if (!plugin->Check(*data))
      {
        continue;
      }
      if (std::size_t usedSize = plugin->Process(location, callback))
      {
        return usedSize;
      }
    }
    return 0;
  }

  std::size_t DetectArchive(DataLocation::Ptr location, const Module::DetectCallback& callback)
  {
    const PluginsEnumerator::Ptr usedPlugins = callback.GetUsedPlugins();
    const IO::DataContainer::Ptr data = location->GetData();
    const Parameters::Accessor::Ptr parameters = callback.GetPluginsParameters();
    for (ArchivePlugin::Iterator::Ptr iter = usedPlugins->EnumerateArchives(); iter->IsValid(); iter->Next())
    {
      const ArchivePlugin::Ptr plugin = iter->Get();
      if (!plugin->Check(*data))
      {
        continue;
      }
      //TODO: adapt to new
      std::size_t usedSize = 0;
      if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(*parameters, *data, usedSize))
      {
        const String subpath = EncodeArchivePluginToPath(plugin->Id());
        const DataLocation::Ptr subLocation = CreateNestedLocation(location, plugin, subdata, subpath);
        Module::Detect(subLocation, callback);
        return usedSize;
      }
      //TODO: dispatch heavy checks- return false if not enabled
    }
    return 0;
  }

  std::size_t DetectModule(DataLocation::Ptr location, const Module::DetectCallback& callback)
  {
    const PluginsEnumerator::Ptr usedPlugins = callback.GetUsedPlugins();
    const IO::DataContainer::Ptr data = location->GetData();
    const MetaContainer input = MetaContainerFromLocation(*location);
    for (PlayerPlugin::Iterator::Ptr plugins = usedPlugins->EnumeratePlayers(); plugins->IsValid(); plugins->Next())
    {
      const PlayerPlugin::Ptr plugin = plugins->Get();
      if (!plugin->Check(*input.Data))
      {
        continue;
      }
      const Parameters::Accessor::Ptr moduleParams = callback.CreateModuleParameters(*location);
      std::size_t usedSize = 0;
      if (Module::Holder::Ptr module = plugin->CreateModule(moduleParams, input, usedSize))
      {
        ThrowIfError(callback.ProcessModule(*location, module));
        return usedSize;
      }
      //TODO: dispatch heavy checks- return false if not enabled
    }
    return 0;
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
}

namespace ZXTune
{
  namespace Module
  {
    Holder::Ptr Open(DataLocation::Ptr location, PluginsEnumerator::Ptr usedPlugins, Parameters::Accessor::Ptr moduleParams)
    {
      const OpenModuleCallback callback(usedPlugins, moduleParams);
      DetectModule(location, callback);
      return callback.GetResult();
    }

    std::size_t Detect(DataLocation::Ptr location, const DetectCallback& callback)
    {
      if (std::size_t usedSize = DetectContainer(location, callback))
      {
        return usedSize;
      }
      if (std::size_t usedSize = DetectArchive(location, callback))
      {
        return usedSize;
      }
      return DetectModule(location, callback);
    }
  }
}
