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
#include <tools.h>
//library includes
#include <core/module_detect.h>
#include <debug/log.h>
#include <l10n/api.h>
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

  const Debug::Stream Dbg("Core::Enumerator");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");

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
    }

    virtual typename PluginType::Iterator::Ptr Enumerate() const
    {
      return CreateRangedObjectIteratorAdapter(Plugins.begin(), Plugins.end());
    }
  private:
    std::vector<typename PluginType::Ptr> Plugins;
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

    virtual void ProcessModule(DataLocation::Ptr location, Module::Holder::Ptr holder) const
    {
      const Analysis::Path::Ptr subPath = location->GetPath();
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
}

namespace ZXTune
{
  template<>
  ArchivePluginsEnumerator::Ptr ArchivePluginsEnumerator::Create()
  {
    static ArchivePluginsContainer instance;
    return ArchivePluginsEnumerator::Ptr(&instance, NullDeleter<ArchivePluginsEnumerator>());
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

  Plugin::Ptr CreatePluginDescription(const String& id, const String& info, uint_t capabilities)
  {
    return boost::make_shared<SimplePluginDescription>(id, info, capabilities);
  }

  void DetectModules(Parameters::Accessor::Ptr pluginsParams, const DetectParameters& detectParams, Binary::Container::Ptr data)
  {
    if (!data.get())
    {
      throw Error(THIS_LINE, translate("Invalid parameters specified."));
    }
    const DataLocation::Ptr location = CreateLocation(pluginsParams, data);
    const DetectCallbackAdapter callback(detectParams, pluginsParams);
    Module::Detect(location, callback);
  }

  Module::Holder::Ptr OpenModule(Parameters::Accessor::Ptr pluginsParams, Binary::Container::Ptr data, const String& subpath)
  {
    if (!data.get())
    {
      throw Error(THIS_LINE, translate("Invalid parameters specified."));
    }
    if (const DataLocation::Ptr location = OpenLocation(pluginsParams, data, subpath))
    {
      if (const Module::Holder::Ptr res = Module::Open(location))
      {
        return res;
      }
    }
    throw MakeFormattedError(THIS_LINE,
      translate("Failed to find specified submodule starting from path '%1%'."), subpath);
  }
}
