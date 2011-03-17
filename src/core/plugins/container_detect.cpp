/*
Abstract:
  Module container used for modules detecting

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "core.h"
#include "plugins_types.h"
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
  using namespace Module;

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return ARCHIVE_PLUGIN_PREFIX + pluginId;
  }

  class Subcontainer : public Container
  {
  public:
    Subcontainer(Container::Ptr parent, Plugin::Ptr subPlugin, IO::DataContainer::Ptr subData, const String& subPath)
      : Parent(parent)
      , SubData(subData)
      , SubPlugin(subPlugin)
      , SubPath(subPath)
    {
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      return SubData;
    }

    virtual String GetPath() const
    {
      return IO::AppendPath(Parent->GetPath(), SubPath);
    }

    virtual PluginsChain::ConstPtr GetPlugins() const
    {
      //TODO: optimize
      const PluginsChain::Ptr res = Parent->GetPlugins()->Clone();
      res->Add(SubPlugin);
      return res;
    }
  private:
    const Container::Ptr Parent;
    const IO::DataContainer::Ptr SubData;
    const Plugin::Ptr SubPlugin;
    const String SubPath;
  };

  std::size_t DetectContainer(Container::Ptr container, const DetectCallback& callback)
  {
    const PluginsEnumerator::Ptr usedPlugins = callback.GetEnabledPlugins();
    const IO::DataContainer::Ptr data = container->GetData();
    for (ContainerPlugin::Iterator::Ptr iter = usedPlugins->EnumerateContainers(); iter->IsValid(); iter->Next())
    {
      const ContainerPlugin::Ptr plugin = iter->Get();
      if (!plugin->Check(*data))
      {
        continue;
      }
      if (std::size_t usedSize = plugin->Process(container, callback))
      {
        return usedSize;
      }
    }
    return 0;
  }

  std::size_t DetectArchive(Container::Ptr container, const DetectCallback& callback)
  {
    const PluginsEnumerator::Ptr usedPlugins = callback.GetEnabledPlugins();
    const IO::DataContainer::Ptr data = container->GetData();
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
        const Container::Ptr subcontainer = CreateSubcontainer(container, plugin, subdata, subpath);
        DetectModules(subcontainer, callback);
        return usedSize;
      }
      //TODO: dispatch heavy checks- return false if not enabled
    }
    return 0;
  }

  std::size_t DetectModule(Container::Ptr container, const DetectCallback& callback)
  {
    const PluginsEnumerator::Ptr usedPlugins = callback.GetEnabledPlugins();
    const MetaContainer input = MetaContainerFromContainer(*container);
    const Parameters::Accessor::Ptr moduleParams = callback.GetModuleParameters(*container);
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
        ThrowIfError(callback.ProcessModule(*container, module));
        return usedSize;
      }
      //TODO: dispatch heavy checks- return false if not enabled
    }
    return 0;
  }
}

namespace ZXTune
{
  namespace Module
  {
    std::size_t DetectModules(Container::Ptr container, const DetectCallback& callback)
    {
      if (std::size_t usedSize = DetectContainer(container, callback))
      {
        return usedSize;
      }
      if (std::size_t usedSize = DetectArchive(container, callback))
      {
        return usedSize;
      }
      return DetectModule(container, callback);
    }

    Container::Ptr CreateSubcontainer(Container::Ptr parent, Plugin::Ptr subPlugin, IO::DataContainer::Ptr subData, const String& subPath)
    {
      return boost::make_shared<Subcontainer>(parent, subPlugin, subData, subPath);
    }
  }
}
