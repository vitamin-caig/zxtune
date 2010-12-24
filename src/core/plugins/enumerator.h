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
//std includes
#include <list>
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

    uint_t Checksum(const IO::DataContainer& container) const;
    IO::DataContainer::Ptr Extract(const IO::DataContainer& container) const;
  };

  class PluginsChain
  {
  public:
    typedef boost::shared_ptr<PluginsChain> Ptr;

    virtual ~PluginsChain() {}

    virtual void Add(Plugin::Ptr plugin) = 0;
    virtual Plugin::Ptr GetLast() const = 0;
    virtual PluginsChain::Ptr Clone() const = 0;

    virtual uint_t Count() const = 0;
    virtual String AsString() const = 0;
    virtual uint_t CalculateContainersNesting() const = 0;

    static Ptr Create();
  };

  //module container descriptor- all required data
  struct MetaContainer
  {
    MetaContainer()
      : Plugins(PluginsChain::Create())
    {
    }

    MetaContainer(const MetaContainer& rh)
      : Data(rh.Data)
      , Path(rh.Path)
      , Plugins(rh.Plugins->Clone())
    {
    }

    IO::DataContainer::Ptr Data;
    String Path;
    PluginsChain::Ptr Plugins;
  };

  class PlayerPlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const PlayerPlugin> Ptr;

    //! @brief Checking if data contains module
    //! @return true if possibly yes, false if defenitely no
    virtual bool Check(const IO::DataContainer& inputData) const = 0;
    
    //! @brief Creating module on specified input data
    //! @param parameters Options for modules detection and creation
    //! @param inputData Source memory data
    //! @param region Reference to result region where module is detected
    //! @return Not empty pointer if found, empty elsewhere
    virtual Module::Holder::Ptr CreateModule(Parameters::Accessor::Ptr parameters,
                                             const MetaContainer& inputData,
                                             ModuleRegion& region) const = 0;
  };

  class ArchivePlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const ArchivePlugin> Ptr;

    //! @brief Checking if data contains subdata
    //! @return true if possibly yes, false if defenitely no
    virtual bool Check(const IO::DataContainer& inputData) const = 0;

    //! @brief Extracting subdata from specified input data
    //! @param parameters Options for subdata extraction
    //! @param inputData Source memory data
    //! @param region Reference to result region subdata is extracted from
    //! @return Not empty pointer if data is extracted, empty elsewhere
    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& parameters,
                                                  const MetaContainer& inputData,
                                                  ModuleRegion& region) const = 0;
  };

  class ContainerPlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const ContainerPlugin> Ptr;

    //! @brief Checking if data contains valid subdata
    //! @return true if possibly yes, false if defenitely no
    virtual bool Check(const IO::DataContainer& inputData) const = 0;

    //! @brief Process input data as container
    //! @param parameters Options for processing
    //! @param detectParams Detection-specific parameters
    //! @param inputData Source memory data
    //! @param region Reference to result region subdata container detected at
    //! @return true if processed and region contains busy data
    virtual bool Process(Parameters::Accessor::Ptr parameters,
                          const DetectParameters& detectParams,
                          const MetaContainer& inputData,
                          ModuleRegion& region) const = 0;

    //! @brief Opening subdata by specified path
    //! @param parameters Options for opening
    //! @param inputData Source memory data
    //! @param fullPath Full subdata path
    //! @param restPath Reference to rest part of path which is not handled by current plugin
    //! @return Not empty pointer if data is opened
    virtual IO::DataContainer::Ptr Open(const Parameters::Accessor& parameters,
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
    //archives containers support
    virtual void RegisterPlugin(ArchivePlugin::Ptr plugin) = 0;
    //nested containers support
    virtual void RegisterPlugin(ContainerPlugin::Ptr plugin) = 0;

    //public interface
    virtual Plugin::Iterator::Ptr Enumerate() const = 0;

    //private interface
    //resolve subpath
    virtual void ResolveSubpath(const Parameters::Accessor& coreParams, IO::DataContainer::Ptr data,
      const String& subpath, MetaContainer& result) const = 0;
    //full module detection
    virtual void DetectModules(Parameters::Accessor::Ptr modulesParams, const DetectParameters& params, const MetaContainer& data,
      ModuleRegion& region) const = 0;
    //single module opening
    virtual void OpenModule(Parameters::Accessor::Ptr modulesParams, const MetaContainer& data, 
      Module::Holder::Ptr& holder) const = 0;

    //instantiator
    static PluginsEnumerator& Instance();
  };
}

#endif //__CORE_PLUGINS_ENUMERATOR_H_DEFINED__
