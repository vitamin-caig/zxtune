#include "plugin_enumerator.h"

#include <player_attrs.h>

#include <list>
#include <cassert>
#include <algorithm>
#include <functional>

namespace
{
  using namespace ZXTune;

  struct PluginDescriptor
  {
    CheckFunc Checker;
    FactoryFunc Creator;
    InfoFunc Descriptor;
    //cached
    unsigned Priority;
  };

  unsigned GetPriorityFromCaps(unsigned caps)
  {
    if (caps & CAP_MULTITRACK)
    {
      return 100;
    }
    else if (caps & CAP_CONTAINER)
    {
      return 50;
    }
    return 0;
  }

  bool operator == (const PluginDescriptor& lh, const PluginDescriptor& rh)
  {
    return lh.Checker == rh.Checker && lh.Creator == rh.Creator && lh.Descriptor == rh.Descriptor;
  }
  
  bool operator < (const PluginDescriptor& lh, const PluginDescriptor& rh)
  {
    return lh.Priority < rh.Priority;
  }

  class PluginEnumeratorImpl : public PluginEnumerator
  {
    typedef std::list<PluginDescriptor> PluginsStorage;
  public:
    PluginEnumeratorImpl()
    {
    }

    virtual void RegisterPlugin(CheckFunc check, FactoryFunc create, InfoFunc describe)
    {
      ModulePlayer::Info info;
      describe(info);
      const PluginDescriptor descr = {check, create, describe, GetPriorityFromCaps(info.Capabilities)};
      assert(Plugins.end() == std::find(Plugins.begin(), Plugins.end(), descr));
      Plugins.insert(std::find_if(Plugins.begin(), Plugins.end(), std::bind2nd(std::less<PluginDescriptor>(), descr)), descr);
    }

    virtual void EnumeratePlugins(std::vector<ModulePlayer::Info>& infos) const
    {
      infos.resize(Plugins.size());
      std::vector<ModulePlayer::Info>::iterator out(infos.begin());
      for (PluginsStorage::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it, ++out)
      {
        (it->Descriptor)(*out);
      }
    }

    virtual bool CheckModule(const String& filename, const IO::DataContainer& data, ModulePlayer::Info& info) const
    {
      for (PluginsStorage::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it)
      {
        if ((it->Checker)(filename, data))
        {
          (it->Descriptor)(info);
          return true;
        }
      }
      return false;
    }

    virtual ModulePlayer::Ptr CreatePlayer(const String& filename, const IO::DataContainer& data) const
    {
      for (PluginsStorage::const_iterator it = Plugins.begin(), lim = Plugins.end(); it != lim; ++it)
      {
        if ((it->Checker)(filename, data))
        {
          return (it->Creator)(filename, data);
        }
      }
      return ModulePlayer::Ptr(0);
    }
  private:
    PluginsStorage Plugins;
  };
}

namespace ZXTune
{
  PluginEnumerator& PluginEnumerator::Instance()
  {
    static PluginEnumeratorImpl instance;
    return instance;
  }

  ModulePlayer::Ptr ModulePlayer::Create(const String& filename, const IO::DataContainer& data)
  {
    return PluginEnumerator::Instance().CreatePlayer(filename, data);
  }

  bool ModulePlayer::Check(const String& filename, const IO::DataContainer& data, ModulePlayer::Info& info)
  {
    return PluginEnumerator::Instance().CheckModule(filename, data, info);
  }

  void GetSupportedPlayers(std::vector<ModulePlayer::Info>& infos)
  {
    return PluginEnumerator::Instance().EnumeratePlugins(infos);
  }
}
