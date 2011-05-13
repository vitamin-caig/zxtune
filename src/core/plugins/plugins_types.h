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

  //! Abstract data detection result
  class DetectionResult
  {
  public:
    typedef boost::shared_ptr<const DetectionResult> Ptr;
    virtual ~DetectionResult() {}

    //! @brief Returns data size that is processed in current data position
    //! @return Size of input data format detected
    //! @invariant Return 0 if data is not matched at the beginning
    virtual std::size_t GetMatchedDataSize() const = 0;
    //! @brief Search format in forward data
    //! @return Offset in input data where perform next checking
    //! @invariant Return 0 if current data matches format
    //! @invariant Return input data size if no data at all
    virtual std::size_t GetLookaheadOffset() const = 0;

    static Ptr CreateMatched(std::size_t matchedSize);
    static Ptr CreateUnmatched(DataFormat::Ptr format, IO::DataContainer::Ptr data);
    static Ptr CreateUnmatched(std::size_t unmatchedSize);
  };

  class PlayerPlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const PlayerPlugin> Ptr;
    typedef ObjectIterator<PlayerPlugin::Ptr> Iterator;

    //! @brief Checking if data contains module
    //! @return true if possibly yes, false if defenitely no
    virtual bool Check(const IO::DataContainer& inputData) const = 0;

    //! @brief Detect modules in data
    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const = 0;
  };

  class ArchivePlugin : public Plugin
  {
  public:
    typedef boost::shared_ptr<const ArchivePlugin> Ptr;
    typedef ObjectIterator<ArchivePlugin::Ptr> Iterator;

    //! @brief Detect modules in data
    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const = 0;

    virtual DataLocation::Ptr Open(const Parameters::Accessor& parameters,
                                   DataLocation::Ptr inputData,
                                   const DataPath& pathToOpen) const = 0; 
  };

  //for compatibility
  class ContainerPlugin : public ArchivePlugin
  {
  public:
    typedef boost::shared_ptr<const ContainerPlugin> Ptr;
    typedef ObjectIterator<ContainerPlugin::Ptr> Iterator;
  };
}

#endif //__CORE_PLUGINS_TYPES_H_DEFINED__
