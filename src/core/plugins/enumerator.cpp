/**
 *
 * @file
 *
 * @brief  Plugins container implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archive_plugins_enumerator.h"
#include "core/plugins/archives/plugins_list.h"
#include "core/plugins/player_plugins_enumerator.h"
#include "core/plugins/players/plugins_list.h"
#include "core/plugins/registrator.h"
#include "core/src/l10n.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <debug/log.h>
#include <time/timer.h>

#define FILE_TAG 04EDD719

namespace ZXTune
{
  const Debug::Stream EnumeratorDbg("Core::Enumerator");
  using Module::translate;

  template<class PluginType>
  class Registrator : public PluginsRegistrator<PluginType>
  {
  public:
    Registrator(std::vector<typename PluginType::Ptr>& typed, std::vector<Plugin::Ptr>& all)
      : Typed(typed)
      , All(all)
    {}

    ~Registrator() override
    {
      EnumeratorDbg("Registered %1% plugins for %2%ms", Typed.size(), Timer.Elapsed<Time::Millisecond>().Get());
    }

    void RegisterPlugin(typename PluginType::Ptr plugin) override
    {
      Typed.emplace_back(plugin);
      EnumeratorDbg("Registered %1%", plugin->Id());
      All.emplace_back(std::move(plugin));
    }

  private:
    const Time::Timer Timer;
    std::vector<typename PluginType::Ptr>& Typed;
    std::vector<Plugin::Ptr>& All;
  };

  class AllPlugins
  {
  public:
    static const AllPlugins& Instance()
    {
      static const AllPlugins INSTANCE;
      return INSTANCE;
    }

    std::vector<ArchivePlugin::Ptr> Archives;
    std::vector<PlayerPlugin::Ptr> Players;
    std::vector<Plugin::Ptr> All;

  private:
    AllPlugins()
    {
      Archives.reserve(128);
      Players.reserve(256);
      All.reserve(384);
      RegisterArchivePlugins();
      RegisterPlayerPlugins();
    }

    void RegisterArchivePlugins()
    {
      Registrator<ArchivePlugin> registrator(Archives, All);
      ZXTune::RegisterArchivePlugins(registrator);
    }

    void RegisterPlayerPlugins()
    {
      Registrator<PlayerPlugin> registrator(Players, All);
      ZXTune::RegisterPlayerPlugins(registrator);
    }
  };

  template<>
  const std::vector<ArchivePlugin::Ptr>& ArchivePluginsEnumerator::GetPlugins()
  {
    return AllPlugins::Instance().Archives;
  }

  template<>
  const std::vector<PlayerPlugin::Ptr>& PlayerPluginsEnumerator::GetPlugins()
  {
    return AllPlugins::Instance().Players;
  }

  Plugin::Iterator::Ptr EnumeratePlugins()
  {
    const auto& plugins = AllPlugins::Instance().All;
    return CreateRangedObjectIteratorAdapter(plugins.begin(), plugins.end());
  }
}  // namespace ZXTune

#undef FILE_TAG
