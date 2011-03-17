/*
Abstract:
  Module container used for opening and resolving

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "core.h"
//library includes
#include <io/fs_tools.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  bool IsArchivePluginPathComponent(const String& component)
  {
    return 0 == component.find(ARCHIVE_PLUGIN_PREFIX);
  }

  String DecodeArchivePluginFromPathComponent(const String& component)
  {
    assert(IsArchivePluginPathComponent(component));
    return component.substr(ARCHIVE_PLUGIN_PREFIX.size());
  }

  class ResolvedPluginsIterator
  {
  public:
    ResolvedPluginsIterator(PluginsEnumerator::Ptr plugins, Parameters::Accessor::Ptr coreParams, IO::DataContainer::Ptr data, const String& path)
      : Plugins(plugins)
      , Params(coreParams)
      , Data(data)
      , Path(path)
    {
    }

    bool HasMoreToResolve() const
    {
      return !Path.empty();
    }

    Plugin::Ptr ResolveNextPlugin()
    {
      assert(!Path.empty());
      if (Plugin::Ptr res = ResolveArchivePlugin())
      {
        return res;
      }
      return ResolveContainerPlugin();
    }

    IO::DataContainer::Ptr GetResolvedData() const
    {
      return Data;
    }

    String GetUnresolvedPath() const
    {
      return Path;
    }
  private:
    Plugin::Ptr ResolveArchivePlugin()
    {
      String restPath;
      const String pathComponent = IO::ExtractFirstPathComponent(Path, restPath);
      if (!IsArchivePluginPathComponent(pathComponent))
      {
        return Plugin::Ptr();
      }
      const String pluginId = DecodeArchivePluginFromPathComponent(pathComponent);
      for (ArchivePlugin::Iterator::Ptr iter = Plugins->EnumerateArchives(); iter->IsValid(); iter->Next())
      {
        const ArchivePlugin::Ptr plugin = iter->Get();
        if (plugin->Id() != pluginId)
        {
          continue;
        }
        if (!plugin->Check(*Data))
        {
          continue;
        }
        std::size_t usedSize = 0;
        if (IO::DataContainer::Ptr subdata = plugin->ExtractSubdata(*Params, *Data, usedSize))
        {
          Data = subdata;
          Path = restPath;
          return plugin;
        }
        break;
      }
      return Plugin::Ptr();
    }

    Plugin::Ptr ResolveContainerPlugin()
    {
      for (ContainerPlugin::Iterator::Ptr iter = Plugins->EnumerateContainers(); iter->IsValid(); iter->Next())
      {
        const ContainerPlugin::Ptr plugin = iter->Get();
        if (!plugin->Check(*Data))
        {
          continue;
        }
        String restPath;

        MetaContainer input;
        input.Data = Data;
        input.Path = Path;
        if (IO::DataContainer::Ptr subdata = plugin->Open(*Params, input, Path, restPath))
        {
          //assert(String::npos != Path.rfind(restPath));
          //Log::Debug(THIS_MODULE, "Resolved path components '%1%' with container plugin %2%", Path.substr(0, Path.rfind(restPath)), plugin->Id());
          Data = subdata;
          Path = restPath;
          return plugin;
        }
        //TODO: dispatch heavy checks- break if not enabled
      }
      return Plugin::Ptr();
    }
  private:
    const PluginsEnumerator::Ptr Plugins;
    const Parameters::Accessor::Ptr Params;
    IO::DataContainer::Ptr Data;
    String Path;
  };

  class ResolvedContainer : public Module::Container
  {
  public:
    ResolvedContainer(IO::DataContainer::Ptr data, const String& path, PluginsChain::Ptr plugins)
      : Data(data)
      , Path(path)
      , Plugins(plugins)
    {
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      return Data;
    }

    virtual String GetPath() const
    {
      return Path;
    }

    virtual PluginsChain::ConstPtr GetPlugins() const
    {
      return Plugins;
    }
  private:
    const IO::DataContainer::Ptr Data;
    const String Path;
    const PluginsChain::Ptr Plugins;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Container::Ptr OpenContainer(Parameters::Accessor::Ptr coreParams, IO::DataContainer::Ptr data, const String& subpath)
    {
      const PluginsChain::Ptr resolvedPlugins = PluginsChain::Create();
      if (subpath.empty())
      {
        return boost::make_shared<ResolvedContainer>(data, subpath, resolvedPlugins);
      }

      const PluginsEnumerator::Ptr usedPlugins = PluginsEnumerator::Create();
      ResolvedPluginsIterator iter(usedPlugins, coreParams, data, subpath);
      while (iter.HasMoreToResolve())
      {
        if (const Plugin::Ptr plugin = iter.ResolveNextPlugin())
        {
          resolvedPlugins->Add(plugin);
        }
        else
        {
          //Log::Debug(THIS_MODULE, "Failed to resolve subpath '%1%'", iter.GetUnresolvedPath());
          return Container::Ptr();
        }
      }
      const IO::DataContainer::Ptr resolvedData = iter.GetResolvedData();
      return boost::make_shared<ResolvedContainer>(resolvedData, subpath, resolvedPlugins);
    }

    Holder::Ptr OpenModule(Container::Ptr container, Parameters::Accessor::Ptr moduleParams)
    {
      //TODO: remove
      const MetaContainer input = MetaContainerFromContainer(*container);
      const PluginsEnumerator::Ptr usedPlugins = PluginsEnumerator::Create();
      for (PlayerPlugin::Iterator::Ptr plugins = usedPlugins->EnumeratePlayers(); plugins->IsValid(); plugins->Next())
      {
        const PlayerPlugin::Ptr plugin = plugins->Get();
        if (!plugin->Check(*input.Data))
        {
          continue;//invalid plugin
        }
        std::size_t usedSize = 0;
        if (Module::Holder::Ptr module = plugin->CreateModule(moduleParams, input, usedSize))
        {
          return module;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return Holder::Ptr();
    }
  }
}
