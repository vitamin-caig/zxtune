/*
Abstract:
  Module location used for opening and resolving

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "location.h"
#include "core/plugins/enumerator.h"
//library includes
#include <io/fs_tools.h>
//std includes
#include <vector>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/mem_fn.hpp>
#include <boost/algorithm/string/join.hpp>
//text includes
#include <core/text/core.h>
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
  
  class ResolvedPluginsChain : public PluginsChain
  {
    typedef std::vector<Plugin::Ptr> PluginsList;
  public:
    typedef boost::shared_ptr<ResolvedPluginsChain> Ptr;
  
    void Add(Plugin::Ptr plugin)
    {
      Container.push_back(plugin);
    }

    virtual Plugin::Ptr GetLast() const
    {
      if (!Container.empty())
      {
        return Container.back();
      }
      return Plugin::Ptr();
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
  private:
    PluginsList Container;
  };

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

        if (IO::DataContainer::Ptr subdata = plugin->Open(*Params, *Data, Path, restPath))
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

  class ResolvedLocation : public DataLocation
  {
  public:
    ResolvedLocation(IO::DataContainer::Ptr data, const String& path, PluginsChain::Ptr plugins)
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

    virtual PluginsChain::Ptr GetPlugins() const
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
  DataLocation::Ptr CreateLocation(Parameters::Accessor::Ptr coreParams, IO::DataContainer::Ptr data)
  {
    return OpenLocation(coreParams, data, String());
  }

  DataLocation::Ptr OpenLocation(Parameters::Accessor::Ptr coreParams, IO::DataContainer::Ptr data, const String& subpath)
  {
    const ResolvedPluginsChain::Ptr resolvedPlugins = boost::make_shared<ResolvedPluginsChain>();
    if (subpath.empty())
    {
      return boost::make_shared<ResolvedLocation>(data, subpath, resolvedPlugins);
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
        return DataLocation::Ptr();
      }
    }
    const IO::DataContainer::Ptr resolvedData = iter.GetResolvedData();
    return boost::make_shared<ResolvedLocation>(resolvedData, subpath, resolvedPlugins);
  }
}
