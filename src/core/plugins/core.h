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
#include "container.h"
#include "enumerator.h"
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
  namespace Module
  {
    //! @param coreParams Parameters for plugins processing
    //! @param data Source data to be processed
    //! @param subpath Subpath in source data to be resolved
    //! @return Object if path is valid. No object elsewhere
    Container::Ptr OpenContainer(Parameters::Accessor::Ptr coreParams, IO::DataContainer::Ptr data, const String& subpath);
    //! @param container Source data container
    //! @param moduleParams Parameters to be passed in module
    //! @return Object is exists. No object elsewhere
    Holder::Ptr OpenModule(Container::Ptr container, Parameters::Accessor::Ptr moduleParams);

    class DetectCallback
    {
    public:
      virtual ~DetectCallback() {}

      //! @brief Returns list of plugins enabled to be processed
      virtual PluginsEnumerator::Ptr GetEnabledPlugins() const = 0;
      //! @brief Returns plugins parameters
      virtual Parameters::Accessor::Ptr GetPluginsParameters() const = 0;
      //! @brief Returns parameters for future module
      virtual Parameters::Accessor::Ptr GetModuleParameters(const Container& container) const = 0;
      //! @brief Process module
      virtual Error ProcessModule(const Container& container, Module::Holder::Ptr holder) const = 0;
      //! @brief Logging callback
      virtual Log::ProgressCallback* GetProgressCallback() const = 0;
    };
    //! @param container Source data container
    //! @param params Detect callback
    //! @return Size in bytes of source data processed
    std::size_t DetectModules(Container::Ptr container, const DetectCallback& callback);
  }
}

#endif //__CORE_PLUGINS_CORE_H_DEFINED__
