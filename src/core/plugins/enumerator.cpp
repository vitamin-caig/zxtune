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
#include "core/plugins/archive_plugins_enumerator.h"
#include "core/plugins/player_plugins_enumerator.h"
#include "core/plugins/registrator.h"
#include "core/plugins/archives/plugins_list.h"
#include "core/plugins/players/plugins_list.h"
#include "core/src/callback.h"
#include "core/src/l10n.h"
//common includes
#include <error_tools.h>
#include <pointers.h>
#include <make_ptr.h>
//library includes
#include <core/module_detect.h>
#include <core/module_open.h>
#include <debug/log.h>
#include <module/attributes.h>
#include <time/timer.h>
//std includes
#include <list>
#include <map>
//text includes
#include <core/text/core.h>

#define FILE_TAG 04EDD719

namespace ZXTune
{
  const Debug::Stream EnumeratorDbg("Core::Enumerator");
  using Module::translate;

  template<class PluginType>
  class PluginsContainer : public PluginsRegistrator<PluginType>
                         , public PluginsEnumerator<PluginType>
  {
  public:
    void RegisterPlugin(typename PluginType::Ptr plugin) override
    {
      const Plugin::Ptr description = plugin->GetDescription();
      Plugins.push_back(plugin);
      EnumeratorDbg("Registered %1%", description->Id());
    }

    typename PluginType::Iterator::Ptr Enumerate() const override
    {
      return CreateRangedObjectIteratorAdapter(Plugins.begin(), Plugins.end());
    }
  protected:
    std::vector<typename PluginType::Ptr> Plugins;
  };

  class ArchivePluginsContainer : public PluginsContainer<ArchivePlugin>
  {
  public:
    ArchivePluginsContainer()
    {
      const Time::Timer timer;
      RegisterArchivePlugins(*this);
      EnumeratorDbg("Registered %1% archive plugins for %2%ms", Plugins.size(), timer.Elapsed<Time::Millisecond>().Get());
    }
  };

  class PlayerPluginsContainer : public PluginsContainer<PlayerPlugin>
  {
  public:
    PlayerPluginsContainer()
    {
      const Time::Timer timer;
      RegisterPlayerPlugins(*this);
      EnumeratorDbg("Registered %1% player plugins for %2%ms", Plugins.size(), timer.Elapsed<Time::Millisecond>().Get());
    }
  };

  class SimplePluginDescription : public Plugin
  {
  public:
    SimplePluginDescription(String  id, String  info, uint_t capabilities)
      : ID(std::move(id))
      , Info(std::move(info))
      , Caps(capabilities)
    {
    }

    String Id() const override
    {
      return ID;
    }

    String Description() const override
    {
      return Info;
    }

    uint_t Capabilities() const override
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
      : Archives(std::move(archives))
      , Players(std::move(players))
    {
      Check(Archives);
      Check(Players);
    }

    bool IsValid() const override
    {
      return Archives || Players;
    }

    Plugin::Ptr Get() const override
    {
      return (Archives ? Archives->Get()->GetDescription() : Players->Get()->GetDescription());
    }

    void Next() override
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
      Check(iter);
    }
    
    template<class T>
    void Check(T& iter)
    {
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
    return MakePtr<CompositePluginsIterator>(archives, players);
  }

  Plugin::Ptr CreatePluginDescription(const String& id, const String& info, uint_t capabilities)
  {
    return MakePtr<SimplePluginDescription>(id, info, capabilities);
  }
}

#undef FILE_TAG
