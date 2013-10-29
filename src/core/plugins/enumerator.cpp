/**
* 
* @file
*
* @brief  Plugins container implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "enumerator.h"
#include "registrator.h"
#include "archives/plugins_list.h"
#include "containers/plugins_list.h"
#include "players/plugins_list.h"
#include "core/src/callback.h"
//common includes
#include <error_tools.h>
#include <pointers.h>
//library includes
#include <binary/container_factories.h>
#include <core/convert_parameters.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/module_open.h>
#include <debug/log.h>
#include <l10n/api.h>
//std includes
#include <list>
#include <map>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG 04EDD719

namespace
{
  const Debug::Stream Dbg("Core::Enumerator");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");
}

namespace ZXTune
{
  template<class PluginType>
  class PluginsContainer : public PluginsRegistrator<PluginType>
                         , public PluginsEnumerator<PluginType>
  {
  public:
    virtual void RegisterPlugin(typename PluginType::Ptr plugin)
    {
      const Plugin::Ptr description = plugin->GetDescription();
      Plugins.push_back(plugin);
      Dbg("Registered %1%", description->Id());
      TypeToPlugin[description->Id()] = plugin;
    }

    virtual typename PluginType::Iterator::Ptr Enumerate() const
    {
      return CreateRangedObjectIteratorAdapter(Plugins.begin(), Plugins.end());
    }

    virtual typename PluginType::Ptr Find(const String& id) const
    {
      const typename std::map<String, typename PluginType::Ptr>::const_iterator it = TypeToPlugin.find(id);
      return it != TypeToPlugin.end()
        ? it->second
        : typename PluginType::Ptr();
    }
  private:
    std::vector<typename PluginType::Ptr> Plugins;
    std::map<String, typename PluginType::Ptr> TypeToPlugin;
  };

  class ArchivePluginsContainer : public PluginsContainer<ArchivePlugin>
  {
  public:
    ArchivePluginsContainer()
    {
      RegisterContainerPlugins(*this);
      RegisterArchivePlugins(*this);
    }
  };

  class PlayerPluginsContainer : public PluginsContainer<PlayerPlugin>
  {
  public:
    PlayerPluginsContainer()
    {
      RegisterPlayerPlugins(*this);
    }
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

  class CompositePluginsIterator : public Plugin::Iterator
  {
  public:
    CompositePluginsIterator(ArchivePlugin::Iterator::Ptr archives, PlayerPlugin::Iterator::Ptr players)
      : Archives(archives)
      , Players(players)
    {
    }

    virtual bool IsValid() const
    {
      return Archives || Players;
    }

    virtual Plugin::Ptr Get() const
    {
      return (Archives ? Archives->Get()->GetDescription() : Players->Get()->GetDescription());
    }

    virtual void Next()
    {
      if (Archives)
      {
        Next(Archives);
      }
      else
      {
        Next(Players);
      }
    }
  private:
    template<class T>
    void Next(T& iter)
    {
      iter->Next();
      if (!iter->IsValid())
      {
        iter = T();
      }
    }
  private:
    ArchivePlugin::Iterator::Ptr Archives;
    PlayerPlugin::Iterator::Ptr Players;
  };

  template<>
  ArchivePluginsEnumerator::Ptr ArchivePluginsEnumerator::Create()
  {
    static ArchivePluginsContainer instance;
    return MakeSingletonPointer(instance);
  }

  template<>
  PlayerPluginsEnumerator::Ptr PlayerPluginsEnumerator::Create()
  {
    static PlayerPluginsContainer instance;
    return PlayerPluginsEnumerator::Ptr(&instance, NullDeleter<PlayerPluginsEnumerator>());
  }

  Plugin::Iterator::Ptr EnumeratePlugins()
  {
    const ArchivePlugin::Iterator::Ptr archives = ArchivePluginsEnumerator::Create()->Enumerate();
    const PlayerPlugin::Iterator::Ptr players = PlayerPluginsEnumerator::Create()->Enumerate();
    return boost::make_shared<CompositePluginsIterator>(archives, players);
  }

  Plugin::Ptr FindPlugin(const String& id)
  {
    if (const PlayerPlugin::Ptr player = PlayerPluginsEnumerator::Create()->Find(id))
    {
      return player->GetDescription();
    }
    else if (const ArchivePlugin::Ptr archive = ArchivePluginsEnumerator::Create()->Find(id))
    {
      return archive->GetDescription();
    }
    else
    {
      return Plugin::Ptr();
    }
  }

  Plugin::Ptr CreatePluginDescription(const String& id, const String& info, uint_t capabilities)
  {
    return boost::make_shared<SimplePluginDescription>(id, info, capabilities);
  }
}

namespace Module
{
  Binary::Data::Ptr GetRawData(const Holder& holder)
  {
    std::auto_ptr<Parameters::DataType> data(new Parameters::DataType());
    if (holder.GetModuleProperties()->FindValue(ATTR_CONTENT, *data))
    {
      return Binary::CreateContainer(data);
    }
    throw Error(THIS_LINE, translate("Invalid parameters specified."));
  }
}
