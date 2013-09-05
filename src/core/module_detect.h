/**
*
* @file     core/module_detect.h
* @brief    Modules detecting functionality
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef CORE_MODULE_DETECT_H_DEFINED
#define CORE_MODULE_DETECT_H_DEFINED

//library includes
#include <binary/container.h>
#include <core/data_location.h>
#include <core/module_holder.h>//for Module::Holder::Ptr
#include <parameters/accessor.h>

//forward declarations
namespace Log
{
  class ProgressCallback;
}

//! @brief Global library namespace
namespace Module
{
  class DetectCallback
  {
  public:
    virtual ~DetectCallback() {}

    //! @brief Returns plugins parameters
    virtual Parameters::Accessor::Ptr GetPluginsParameters() const = 0;
    //! @brief Process module
    virtual void ProcessModule(ZXTune::DataLocation::Ptr location, Module::Holder::Ptr holder) const = 0;
    //! @brief Logging callback
    virtual Log::ProgressCallback* GetProgress() const = 0;
  };

  //! @param location Start data location
  //! @param params Detect callback
  //! @return Size in bytes of source data processed
  std::size_t Detect(ZXTune::DataLocation::Ptr location, const DetectCallback& callback);
}

#endif //CORE_MODULE_DETECT_H_DEFINED
