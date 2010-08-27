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

//common includes
#include <string_helpers.h>
//library includes
#include <core/module_detect.h>
#include <core/plugin.h>
//boost includes
#include <boost/function.hpp>

namespace ZXTune
{
  //used module region descriptor
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

  //module container descriptor- all required data
  struct MetaContainer
  {
    IO::DataContainer::Ptr Data;
    String Path;
    StringArray PluginsChain;
  };

  //helper function to fill standard module properties
  void ExtractMetaProperties(const String& type,
                             const MetaContainer& container, const ModuleRegion& region, const ModuleRegion& fixedRegion,
                             Parameters::Map& properties, Dump& rawData);

  class PlayerPlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const PlayerPlugin> Ptr;

    //! @brief Creating module on specified input data
    //! @param parameters Options for modules detection
    //! @param inputData Source memory data
    //! @param region Reference to result region where module is detected
    //! @return Not empty pointer if found, empty elsewhere
    virtual Module::Holder::Ptr CreateModule(const Parameters::Map& parameters,
                                             const MetaContainer& inputData,
                                             ModuleRegion& region) const = 0;
  };

  class ImplicitPlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const ImplicitPlugin> Ptr;

    //! @brief Extracting subdata from specified input data
    //! @param parameters Options for subdata extraction
    //! @param inputData Source memory data
    //! @param region Reference to result region subdata is extracted from
    //! @return Not empty pointer if data is extracted, empty elsewhere
    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Map& parameters,
                                                  const MetaContainer& inputData,
                                                  ModuleRegion& region) const = 0;
  };

  class ContainerPlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const ContainerPlugin> Ptr;

    //! @brief Process input data as container
    //! @param parameters Options for processing
    //! @param detectParams Detection-specific parameters
    //! @param inputData Source memory data
    //! @param region Reference to result region subdata container detected at
    //! @return Error() in case on success, canceled with return callback error
    virtual Error Process(const Parameters::Map& parameters,
                          const DetectParameters& detectParams,
                          const MetaContainer& inputData,
                          ModuleRegion& region) const = 0;

    //! @brief Opening subdata by specified path
    //! @param parameters Options for opening
    //! @param inputData Source memory data
    //! @param fullPath Full subdata path
    //! @param restPath Reference to rest part of path which is not handled by current plugin
    //! @return Not empty pointer if data is opened
    virtual IO::DataContainer::Ptr Open(const Parameters::Map& parameters,
                                        const MetaContainer& inputData,
                                        const String& fullPath,
                                        String& restPath) const = 0;
  };

  class PluginsEnumerator
  {
  public:
    virtual ~PluginsEnumerator() {}

    //endpoint modules support
    virtual void RegisterPlugin(PlayerPlugin::Ptr plugin) = 0;
    //implicit containers support
    virtual void RegisterPlugin(ImplicitPlugin::Ptr plugin) = 0;
    //nested containers support
    virtual void RegisterPlugin(ContainerPlugin::Ptr plugin) = 0;
    //accessing for registered plugin
    virtual const Plugin& GetPluginById(const String& id) const = 0;

    //public interface
    virtual Plugin::Iterator::Ptr Enumerate() const = 0;

    //private interface
    //resolve subpath
    virtual Error ResolveSubpath(const Parameters::Map& commonParams, IO::DataContainer::Ptr data,
      const String& subpath, MetaContainer& result) const = 0;
    //full module detection
    virtual Error DetectModules(const Parameters::Map&, const DetectParameters& params, const MetaContainer& data,
      ModuleRegion& region) const = 0;
    //single module opening
    virtual Error OpenModule(const Parameters::Map& commonParams, const MetaContainer& data, 
      Module::Holder::Ptr& holder) const = 0;

    //instantiator
    static PluginsEnumerator& Instance();
  };

  //calculate container plugins in chain
  uint_t CalculateContainersNesting(const StringArray& pluginsChain);
}

#endif //__CORE_PLUGINS_ENUMERATOR_H_DEFINED__
