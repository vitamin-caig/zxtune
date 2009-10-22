/*
Abstract:
  Plugins enumerator interface and related

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_ENUMERATOR_H_DEFINED__
#define __CORE_PLUGINS_ENUMERATOR_H_DEFINED__

#include "../player.h"

#include <boost/function.hpp>

namespace ZXTune
{
  namespace IO
  {
    class FastDump;
  }
  
  struct ModuleRegion
  {
    ModuleRegion() : Offset(), Size()
    {
    }
    std::size_t Offset;
    std::size_t Size;
  };
  
  typedef boost::function<bool(const String&, const IO::FastDump&, ModuleRegion&, Module::Player::Ptr&)> CreatePlayerFunc;

  class PluginsEnumerator
  {
  public:
    virtual ~PluginsEnumerator() {}

    //endpoint modules support
    virtual void RegisterPlayerPlugin(const PluginInformation& info, const CreatePlayerFunc& func) = 0;

    
    //public interface
    virtual void EnumeratePlugins(std::vector<PluginInformation>& plugins) const = 0;
    virtual Error DetectModules(const IO::DataContainer& data, const DetectParameters& params) const = 0;
    
    static PluginsEnumerator& Instance();
  };
}

#endif //__CORE_PLUGINS_ENUMERATOR_H_DEFINED__
