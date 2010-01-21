/*
Abstract:
  Modules detecting

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
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

namespace ZXTune
{
  //forward declarations
  struct PluginInformation;

  /// Creating
  struct DetectParameters
  {
    typedef boost::function<bool(const PluginInformation&)> FilterFunc;
    FilterFunc Filter;
    typedef boost::function<Error(const String&, Module::Holder::Ptr player)> CallbackFunc;
    CallbackFunc Callback;
    typedef boost::function<void(const String&)> LogFunc;
    LogFunc Logger;
  };

  Error DetectModules(const Parameters::Map& commonParams, const DetectParameters& detectParams,
    IO::DataContainer::Ptr data, const String& startSubpath);
}

#endif //__CORE_MODULE_DETECT_H_DEFINED__
