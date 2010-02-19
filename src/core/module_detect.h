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

//for typedef'ed Parameters::Map
#include <parameters.h>
//for IO::DataContainer::Ptr
#include <io/container.h>
//for Module::Holder::Ptr
#include <core/module_holder.h>

#include <boost/function.hpp>

class Error;

namespace Log
{
  struct MessageData;
}

//! @brief Global library namespace
namespace ZXTune
{
  //forward declarations
  struct PluginInformation;

  //! @brief Paremeters for modules' detection
  struct DetectParameters
  {
    typedef boost::function<bool(const PluginInformation&)> FilterFunc;
    //! Filter. If empty or returns false, specified plugin is processes. Optional
    FilterFunc Filter;
    typedef boost::function<Error(const String&, Module::Holder::Ptr player)> CallbackFunc;
    //! Called on each detected module. Passed subpath and Module#Holder object. Return nonempty error to cancel processing. Mandatory
    CallbackFunc Callback;
    typedef boost::function<void(const Log::MessageData&)> LogFunc;
    //! Simple logger callback. Optional
    LogFunc Logger;
  };

  //! @brief Perform module detection
  //! @param commonParams Detection and modules' construction parameters
  //! @param detectParams %Parameters set
  //! @param data Input data container
  //! @param startSubpath Path in input data to start detecting
  //! @return Error() in case of success
  //! @return ERROR_DETECT_CANCELED with suberror, returned from DetectParameters#Callback
  Error DetectModules(const Parameters::Map& commonParams, const DetectParameters& detectParams,
    IO::DataContainer::Ptr data, const String& startSubpath);
}

#endif //__CORE_MODULE_DETECT_H_DEFINED__
