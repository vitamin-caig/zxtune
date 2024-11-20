/**
 *
 * @file
 *
 * @brief  Plugins container implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archive_plugin.h"
#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/archives/plugins_list.h"
#include "core/plugins/player_plugin.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugins_list.h"

#include "debug/log.h"
#include "time/timer.h"

#include "string_view.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace ZXTune
{
  const Debug::Stream EnumeratorDbg("Core::Enumerator");

  class AllPlugins
    : private ArchivePluginsRegistrator
    , private PlayerPluginsRegistrator
  {
  public:
    static const AllPlugins& Instance()
    {
      static const AllPlugins INSTANCE;
      return INSTANCE;
    }

    void Enumerate(PluginVisitor& visitor) const
    {
      for (const auto& arch : Archives)
      {
        visitor.Visit(*arch);
      }
      for (const auto& play : Players)
      {
        visitor.Visit(*play);
      }
    }

    std::vector<ArchivePlugin::Ptr> Archives;
    std::vector<PlayerPlugin::Ptr> Players;

  private:
    AllPlugins()
    {
      Archives.reserve(128);
      Players.reserve(256);
      const Time::Timer timer;
      ZXTune::RegisterArchivePlugins(*this);
      ZXTune::RegisterPlayerPlugins(*this, *this);
      EnumeratorDbg("Registered {} archives and {} players for {}ms", Archives.size(), Players.size(),
                    timer.Elapsed<Time::Millisecond>().Get());
    }

    void RegisterPlugin(ArchivePlugin::Ptr plugin) override
    {
      EnumeratorDbg("Registered archive {}", plugin->Id());
      Archives.emplace_back(std::move(plugin));
    }

    void RegisterPlugin(PlayerPlugin::Ptr plugin) override
    {
      EnumeratorDbg("Registered player {}", plugin->Id());
      Players.emplace_back(std::move(plugin));
    }
  };

  const std::vector<ArchivePlugin::Ptr>& ArchivePlugin::Enumerate()
  {
    return AllPlugins::Instance().Archives;
  }

  const std::vector<PlayerPlugin::Ptr>& PlayerPlugin::Enumerate()
  {
    return AllPlugins::Instance().Players;
  }

  void EnumeratePlugins(PluginVisitor& visitor)
  {
    AllPlugins::Instance().Enumerate(visitor);
  }
}  // namespace ZXTune
