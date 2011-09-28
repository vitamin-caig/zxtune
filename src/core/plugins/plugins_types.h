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
#include "detection_result.h"
#include "core/src/location.h"
//library includes
#include <core/plugin.h>

namespace ZXTune
{
  namespace Module
  {
    class DetectCallback;
  }
  class PlayerPlugin
  {
  public:
    typedef boost::shared_ptr<const PlayerPlugin> Ptr;
    typedef ObjectIterator<PlayerPlugin::Ptr> Iterator;
    virtual ~PlayerPlugin() {}

    virtual Plugin::Ptr GetDescription() const = 0; 

    //! @brief Detect modules in data
    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const = 0;
  };

  class ArchivePlugin
  {
  public:
    typedef boost::shared_ptr<const ArchivePlugin> Ptr;
    typedef ObjectIterator<ArchivePlugin::Ptr> Iterator;
    virtual ~ArchivePlugin() {}

    virtual Plugin::Ptr GetDescription() const = 0;

    //! @brief Detect modules in data
    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const = 0;

    virtual DataLocation::Ptr Open(const Parameters::Accessor& parameters,
                                   DataLocation::Ptr inputData,
                                   const DataPath& pathToOpen) const = 0; 
  };

  Plugin::Ptr CreatePluginDescription(const String& id, const String& info, uint_t capabilities);
}

#endif //__CORE_PLUGINS_TYPES_H_DEFINED__
