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

#include <string_helpers.h>
#include <core/module_detect.h>
#include <core/plugin.h>

#include <boost/function.hpp>

namespace ZXTune
{
  struct ModuleRegion
  {
    ModuleRegion() : Offset(), Size()
    {
    }
    ModuleRegion(std::size_t off, std::size_t sz)
      : Offset(off), Size(sz)
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

  void ExtractMetaProperties(const String& type,
                             const MetaContainer& container, const ModuleRegion& region, const ModuleRegion& fixedRegion,
                             Parameters::Map& properties, Dump& rawData);

  //in: metacontainer
  //out: holder, region
  typedef boost::function<bool(const Parameters::Map&, const MetaContainer&, Module::Holder::Ptr&, ModuleRegion&)> CreateModuleFunc;
  //in: data
  //output: data, region
  typedef boost::function<bool(const Parameters::Map&, const MetaContainer&, IO::DataContainer::Ptr&, ModuleRegion&)> ProcessImplicitFunc;
  //in: metacontainer, parameters+callback
  //out: region
  typedef boost::function<Error(const Parameters::Map&, const DetectParameters&, const MetaContainer&, ModuleRegion&)> ProcessContainerFunc;
  //in: container, path
  //out: container, rest path
  typedef boost::function<bool(const Parameters::Map&, const MetaContainer&, const String&, IO::DataContainer::Ptr&, String&)> OpenContainerFunc;

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
    virtual void Enumerate(PluginInformationArray& plugins) const = 0;

    //private interface
    //resolve subpath
    virtual Error ResolveSubpath(const Parameters::Map& commonParams, IO::DataContainer::Ptr data,
      const String& subpath, const DetectParameters::LogFunc& logger, MetaContainer& result) const = 0;
    //full module detection
    virtual Error DetectModules(const Parameters::Map&, const DetectParameters& params, const MetaContainer& data,
      ModuleRegion& region) const = 0;

    //instantiator
    static PluginsEnumerator& Instance();
  };
}

#endif //__CORE_PLUGINS_ENUMERATOR_H_DEFINED__
