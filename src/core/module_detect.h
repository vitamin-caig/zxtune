/**
*
* @file     core/module_detect.h
* @brief    Modules detecting functionality
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_MODULE_DETECT_H_DEFINED__
#define __CORE_MODULE_DETECT_H_DEFINED__

//library includes
#include <io/container.h>//for IO::DataContainer::Ptr
#include <core/module_holder.h>//for Module::Holder::Ptr

//forward declarations
class Error;
namespace Log
{
  struct MessageData;
}

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

    //! @brief Processed plugins' filter
    //! @param plugin Reference to plugin intended to be processed
    //! @return true to skip false to process
    virtual bool FilterPlugin(const Plugin& plugin) const = 0;
    //! @brief Called on each detected module.
    //! @param subpath Subpath for processed module
    //! @param holder Found module
    //! @return Error() to continue, else to cancel.
    virtual Error ProcessModule(const String& subpath, Module::Holder::Ptr holder) const = 0;
    //! @brief Logging callback
    //! @param message %Log message to report
    virtual void ReportMessage(const Log::MessageData& message) const = 0;
  };

  //! @brief Perform module detection
  //! @param modulesParams Detection and modules' construction parameters
  //! @param detectParams %Parameters set
  //! @param data Input data container
  //! @param startSubpath Path in input data to start detecting
  //! @return Error() in case of success
  //! @return ERROR_DETECT_CANCELED with suberror, returned from DetectParameters#Callback
  Error DetectModules(Parameters::Accessor::Ptr modulesParams, const DetectParameters& detectParams,
    IO::DataContainer::Ptr data, const String& startSubpath);

  //! @brief Perform single module opening
  //! @param moduleParams Opening and module's construction parameters
  //! @param data Input data container
  //! @param subpath Path in input data to open
  //! @param result Reference to result module
  //! @return Error() in case of success and module is found
  //! @return ERROR_FIND_SUBMODULE in case if module is not found
  Error OpenModule(Parameters::Accessor::Ptr moduleParams, IO::DataContainer::Ptr data, const String& subpath,
    Module::Holder::Ptr& result);
}

#endif //__CORE_MODULE_DETECT_H_DEFINED__
