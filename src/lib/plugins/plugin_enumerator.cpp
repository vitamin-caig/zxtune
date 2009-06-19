#include "plugin_enumerator.h"

#include <error.h>
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
    ModuleCheckFunc Checker;
    ModuleFactoryFunc Creator;
    ModuleInfoFunc Descriptor;
    //cached
    unsigned Priority;
  };

  unsigned GetPriorityFromCaps(unsigned caps)
  {
    switch (caps & (CAP_MULTITRACK | CAP_CONTAINER | CAP_SCANER))
    {
    case CAP_MULTITRACK:
      return 100;
    case CAP_SCANER:
      return 66;
    case CAP_CONTAINER:
      return 33;
    default:
      return 0;
    }
  }

  inline bool operator == (const PluginDescriptor& lh, const PluginDescriptor& rh)
  {
    return lh.Checker == rh.Checker && lh.Creator == rh.Creator && lh.Descriptor == rh.Descriptor;
  }
  
  inline bool operator < (const PluginDescriptor& lh, const PluginDescriptor& rh)
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

    virtual void RegisterPlugin(ModuleCheckFunc check, ModuleFactoryFunc create, ModuleInfoFunc describe)
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
          try
          {
            return (it->Creator)(filename, data);
          }
          catch (const Error& /*e*/)//just skip in case of detailed check fail
          {
            //TODO: log
          }
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
