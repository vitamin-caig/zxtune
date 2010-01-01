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
  struct ModuleRegion
  {
    ModuleRegion() : Offset(), Size()
    {
    }
    std::size_t Offset;
    std::size_t Size;
  };

  struct MetaContainer
  {
    IO::DataContainer::Ptr Data;
    String Path;
    StringArray PluginsChain;
  };
  
  void ExtractMetaProperties(const MetaContainer& container, const ModuleRegion& region,
                             ParametersMap& properties, Dump& rawData);
  
  //in: metacontainer
  //out: holder, region
  typedef boost::function<bool(const ParametersMap&, const MetaContainer&, Module::Holder::Ptr&, ModuleRegion&)> CreateModuleFunc;
  //in: data
  //output: data, region
  typedef boost::function<bool(const ParametersMap&, const MetaContainer&, IO::DataContainer::Ptr&, ModuleRegion&)> ProcessImplicitFunc;
  //in: metacontainer, parameters+callback
  //out: region
  typedef boost::function<Error(const ParametersMap&, const DetectParameters&, const MetaContainer&, ModuleRegion&)> ProcessContainerFunc;
  //in: container, path
  //out: container, rest path
  typedef boost::function<bool(const ParametersMap&, const MetaContainer&, const String&, IO::DataContainer::Ptr&, String&)> OpenContainerFunc;

  class PluginsEnumerator
  {
  public:
    virtual ~PluginsEnumerator() {}

    //endpoint modules support
    virtual void RegisterPlayerPlugin(const PluginInformation& info, const CreateModuleFunc& func) = 0;
    //implicit containers support
    virtual void RegisterImplicitPlugin(const PluginInformation& info, const ProcessImplicitFunc& func) = 0;
    //nested containers support
    virtual void RegisterContainerPlugin(const PluginInformation& info,
      const OpenContainerFunc& opener, const ProcessContainerFunc& processor) = 0;
    
    //public interface
    virtual void EnumeratePlugins(std::vector<PluginInformation>& plugins) const = 0;
    
    //private interface
    //resolve subpath
    virtual Error ResolveSubpath(const ParametersMap& commonParams, IO::DataContainer::Ptr data, 
      const String& subpath, const DetectParameters::LogFunc& logger, MetaContainer& result) const = 0;
    //full module detection
    virtual Error DetectModules(const ParametersMap&, const DetectParameters& params, const MetaContainer& data,
      ModuleRegion& region) const = 0;

    //instantiator
    static PluginsEnumerator& Instance();
  };
}

#endif //__CORE_PLUGINS_ENUMERATOR_H_DEFINED__
