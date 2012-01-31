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

//common includes
#include <logging.h>
//library includes
#include <binary/container.h>
#include <core/module_holder.h>//for Module::Holder::Ptr

//forward declarations
class Error;

namespace Parameters
{
  class Accessor;
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
  //! @param startSubpath Path in input data to start detecting
  //! @return Error() in case of success
  //! @return ERROR_DETECT_CANCELED with suberror, throwed from DetectParameters#Callback
  Error DetectModules(Parameters::Accessor::Ptr pluginsParams, const DetectParameters& detectParams,
    Binary::Container::Ptr data, const String& startSubpath);

  //! @brief Perform single module opening
  //! @param pluginsParams Opening parameters
  //! @param data Input data container
  //! @param subpath Path in input data to open
  //! @param result Reference to result module
  //! @return Error() in case of success and module is found
  //! @return ERROR_FIND_SUBMODULE in case if module is not found
  Error OpenModule(Parameters::Accessor::Ptr pluginsParams, Binary::Container::Ptr data, const String& subpath,
    Module::Holder::Ptr& result);
}

#endif //__CORE_MODULE_DETECT_H_DEFINED__
