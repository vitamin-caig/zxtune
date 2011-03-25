/*
Abstract:
  Module container and related interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CORE_H_DEFINED__
#define __CORE_PLUGINS_CORE_H_DEFINED__

//local includes
#include "location.h"
#include "core/plugins/enumerator.h"
//common includes
#include <parameters.h>
//library includes
#include <core/module_holder.h>

namespace Log
{
  class ProgressCallback;
}

namespace ZXTune
{
  class DataProcessor
  {
  public:
    typedef boost::shared_ptr<const DataProcessor> Ptr;
    virtual ~DataProcessor() {}

    virtual std::size_t ProcessContainers() const = 0;
    virtual std::size_t ProcessArchives() const = 0;
    virtual std::size_t ProcessModules() const = 0;

    static Ptr Create(DataLocation::Ptr location, const Module::DetectCallback& callback);
  };

  namespace Module
  {
    //! @param location Source data location
    //! @param usedPlugins Plugins that can be used for opening
    //! @param moduleParams Parameters to be passed in module
    //! @return Object is exists. No object elsewhere
    Holder::Ptr Open(DataLocation::Ptr location, PluginsEnumerator::Ptr usedPlugins, Parameters::Accessor::Ptr moduleParams);

    //! @param location Start data location
    //! @param params Detect callback
    //! @return Size in bytes of source data processed
    std::size_t Detect(DataLocation::Ptr location, const DetectCallback& callback);
    std::size_t Detect(const DataProcessor& detector);
  }
}

#endif //__CORE_PLUGINS_CORE_H_DEFINED__
