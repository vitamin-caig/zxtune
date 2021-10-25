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
#include "core/plugins/archives/plugins_list.h"
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
    explicit Registrator(std::vector<typename PluginType::Ptr>& typed)
      : Typed(typed)
    {}

    ~Registrator() override
    {
      EnumeratorDbg("Registered %1% plugins for %2%ms", Typed.size(), Timer.Elapsed<Time::Millisecond>().Get());
    }

    void RegisterPlugin(typename PluginType::Ptr plugin) override
    {
      EnumeratorDbg("Registered %1%", plugin->Id());
      Typed.emplace_back(std::move(plugin));
    }

  private:
    const Time::Timer Timer;
    std::vector<typename PluginType::Ptr>& Typed;
  };

  class AllPlugins
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
      RegisterArchivePlugins();
      RegisterPlayerPlugins();
    }

    void RegisterArchivePlugins()
    {
      Registrator<ArchivePlugin> registrator(Archives);
      ZXTune::RegisterArchivePlugins(registrator);
    }

    void RegisterPlayerPlugins()
    {
      Registrator<PlayerPlugin> registrator(Players);
      ZXTune::RegisterPlayerPlugins(registrator);
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

#undef FILE_TAG
