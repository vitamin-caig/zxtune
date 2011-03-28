/*
Abstract:
  Plugins types definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_TYPES_H_DEFINED__
#define __CORE_PLUGINS_TYPES_H_DEFINED__

//local includes
#include "core/src/location.h"
//common includes
#include <detector.h>
#include <types.h>
//library includes
#include <core/module_holder.h>
#include <core/plugin.h>
#include <io/container.h>

namespace ZXTune
{
  namespace Module
  {
    class DetectCallback;
  }

  class PlayerPlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const PlayerPlugin> Ptr;
    typedef ObjectIterator<PlayerPlugin::Ptr> Iterator;

    //! @brief Checking if data contains module
    //! @return true if possibly yes, false if defenitely no
    virtual bool Check(const IO::DataContainer& inputData) const = 0;

    //! @brief Creating module on specified input data
    //! @param parameters Options for modules detection and creation
    //! @param inputData Source memory data
    //! @param usedSize Reference to result data size used to create
    //! @return Not empty pointer if found, empty elsewhere
    virtual Module::Holder::Ptr CreateModule(Parameters::Accessor::Ptr parameters,
                                             DataLocation::Ptr inputData,
                                             std::size_t& usedSize) const = 0;
  };

  class ArchivePlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const ArchivePlugin> Ptr;
    typedef ObjectIterator<ArchivePlugin::Ptr> Iterator;

    //! @brief Checking if data contains subdata
    //! @return true if possibly yes, false if defenitely no
    virtual bool Check(const IO::DataContainer& inputData) const = 0;

    //! @brief Extracting subdata from specified input data
    //! @param parameters Options for subdata extraction
    //! @param inputData Source memory data
    //! @param usedSize Reference to result data size used to extract
    //! @return Not empty pointer if data is extracted, empty elsewhere
    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& parameters,
                                                  const IO::DataContainer& inputData,
                                                  std::size_t& usedSize) const = 0;
  };

  class ContainerPlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const ContainerPlugin> Ptr;
    typedef ObjectIterator<ContainerPlugin::Ptr> Iterator;

    //! @brief Checking if data contains valid subdata
    //! @return true if possibly yes, false if defenitely no
    virtual bool Check(const IO::DataContainer& inputData) const = 0;

    //! @brief Detect modules in data
    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const = 0;

    //! @brief Opening subdata by specified path
    //! @param parameters Options for opening
    //! @param inputData Source memory data
    //! @param fullPath Full subdata path
    //! @param restPath Reference to rest part of path which is not handled by current plugin
    //! @return Not empty pointer if data is opened
    virtual IO::DataContainer::Ptr Open(const Parameters::Accessor& parameters,
                                        const IO::DataContainer& inputData,
                                        const String& fullPath,
                                        String& restPath) const = 0;
  };
}

#endif //__CORE_PLUGINS_TYPES_H_DEFINED__
