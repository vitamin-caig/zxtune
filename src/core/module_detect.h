/**
*
* @file     core/module_detect.h
* @brief    Modules detecting functionality
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __CORE_MODULE_DETECT_H_DEFINED__
#define __CORE_MODULE_DETECT_H_DEFINED__

//library includes
#include <binary/container.h>
#include <core/module_holder.h>//for Module::Holder::Ptr

//forward declarations
namespace Parameters
{
  class Accessor;
}

namespace Log
{
  class ProgressCallback;
}

//! @brief Global library namespace
namespace ZXTune
{
  //forward declarations
  class Plugin;

  //! @brief Paremeters for modules' detection
  class DetectParameters
  {
  public:
    virtual ~DetectParameters() {}

    //! @brief Called on each detected module.
    //! @param subpath Subpath for processed module
    //! @param holder Found module
    virtual void ProcessModule(const String& subpath, Module::Holder::Ptr holder) const = 0;

    //! @brief Request for progress callback
    //! @return 0 if client doesn't want to receive progress notifications
    virtual Log::ProgressCallback* GetProgressCallback() const = 0;
  };

  //! @brief Perform module detection
  //! @param pluginsParams Detection parameters
  //! @param detectParams %Parameters set
  //! @param data Input data container
  //! @throw Error in case of error
  void DetectModules(Parameters::Accessor::Ptr pluginsParams, const DetectParameters& detectParams, Binary::Container::Ptr data);

  //! @brief Perform single module opening
  //! @param pluginsParams Opening parameters
  //! @param data Input data container
  //! @param subpath Path in input data to open
  //! @return Result module
  //! @throw Error in case of error
  Module::Holder::Ptr OpenModule(Parameters::Accessor::Ptr pluginsParams, Binary::Container::Ptr data, const String& subpath);
}

#endif //__CORE_MODULE_DETECT_H_DEFINED__
