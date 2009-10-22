/*
Abstract:
  Plugins enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "enumerator.h"
#include "../plugin.h"

#include "players/plugins_list.h"

#include <io/container.h>

namespace
{
  using namespace ZXTune;

  typedef std::pair<PluginInformation, CreatePlayerFunc> PlayerPluginDescriptor;

  const std::size_t MINIMAL_REGION_SIZE = 64;
  
  class PluginsEnumeratorImpl : public PluginsEnumerator
  {
  public:
    PluginsEnumeratorImpl()
    {
      RegisterPlayerPlugins(*this);
    }
    
    virtual void RegisterPlayerPlugin(const PluginInformation& info, const CreatePlayerFunc& func)
    {
      AllPlugins.push_back(info);
      PlayerPlugins.push_back(PlayerPluginDescriptor(info, func));
    }
    
    //public interface
    virtual void EnumeratePlugins(std::vector<PluginInformation>& plugins) const
    {
      plugins = AllPlugins;
    }
    
    virtual Error DetectModules(const IO::DataContainer& data, const DetectParameters& params) const
    {
      try
      {
        ScanForPlayers(data, params);
        return Error();//success
      }
      catch (const Error& e)
      {
        return e;
      }
    }
    
  private:
    void ScanForPlayers(const IO::DataContainer& data, const DetectParameters& params) const
    {
      for (std::size_t offset = 0, limit = data.Size() - MINIMAL_REGION_SIZE; offset < limit; )
      {
        std::size_t delta(1);
        ModuleRegion region;
        Module::Player::Ptr player;
        if (FindPlayerPlugin(params.Subpath, IO::FastDump(data, offset), params.Filter, region, player))
        {
          ThrowIfError(params.Callback(params.Subpath, player));
          delta = region.Offset + region.Size;
        }
        offset += delta;
      }
    }
    
    bool FindPlayerPlugin(const String& filename, const IO::FastDump& data, const DetectParameters::FilterFunc& filter,
                          ModuleRegion& region, Module::Player::Ptr& player) const
    {
      for (std::vector<PlayerPluginDescriptor>::const_iterator it = PlayerPlugins.begin(), lim = PlayerPlugins.end();
           it != lim; ++it)
      {
        if (filter(it->first) && (it->second)(filename, data, region, player))
        {
          return true;
        }
      }
      return false;
    }
  private:
    std::vector<PluginInformation> AllPlugins;
    std::vector<PlayerPluginDescriptor> PlayerPlugins;
  };
}

namespace ZXTune
{
  PluginsEnumerator& PluginsEnumerator::Instance()
  {
    static PluginsEnumeratorImpl instance;
    return instance;
  }

  void GetSupportedPlugins(std::vector<PluginInformation>& plugins)
  {
    PluginsEnumerator::Instance().EnumeratePlugins(plugins);
  }

  Error DetectModules(const IO::DataContainer& data, const DetectParameters& params)
  {
    return PluginsEnumerator::Instance().DetectModules(data, params);
  }
}
