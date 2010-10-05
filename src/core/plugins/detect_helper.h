/*
Abstract:
  Module detect helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_DETECT_HELPER_H_DEFINED__
#define __CORE_PLUGINS_DETECT_HELPER_H_DEFINED__

//local includes
#include "enumerator.h"
//common includes
#include <detector.h>
//library includes
#include <core/module_holder.h>
//boost includes
#include <boost/function.hpp>

namespace ZXTune
{
  struct DetectFormatChain
  {
    //no default ctor to declare as a structure
    const std::string PlayerFP;
    const std::size_t PlayerSize;
  };
  
  typedef boost::function<bool(const uint8_t*, std::size_t)> Checker;
  typedef boost::function<Module::Holder::Ptr(Plugin::Ptr, Parameters::Accessor::Ptr, const MetaContainer&, ModuleRegion&)> Creator;

  inline bool PerformCheck(const Checker& checker,
    const DetectFormatChain* chainBegin, const DetectFormatChain* chainEnd,
    const IO::DataContainer& container)
  {
    const uint8_t* const data = static_cast<const uint8_t*>(container.Data());
    const std::size_t limit = container.Size();
    
    //detect without
    if (checker(data, limit))
    {
      return true;
    }
    for (const DetectFormatChain* chain = chainBegin; chain != chainEnd; ++chain)
    {
      if (DetectFormat(data, limit, chain->PlayerFP) &&
          checker(data + chain->PlayerSize, limit - chain->PlayerSize))
      {
        return true;
      }
    }
    return false;
  }
  
  inline Module::Holder::Ptr PerformCreate(
    const Checker& checker, const Creator& creator,
    const DetectFormatChain* chainBegin, const DetectFormatChain* chainEnd,
    Plugin::Ptr plugin, Parameters::Accessor::Ptr params, const MetaContainer& container, ModuleRegion& region)
  {
    const std::size_t limit(container.Data->Size());
    const uint8_t* const data(static_cast<const uint8_t*>(container.Data->Data()));

    ModuleRegion tmpRegion;
    //try to detect without player
    if (checker(data, limit))
    {
      if (Module::Holder::Ptr holder = creator(plugin, params, container, tmpRegion))
      {
        region = tmpRegion;
        return holder;
      }
    }
    for (const DetectFormatChain* chain = chainBegin; chain != chainEnd; ++chain)
    {
      tmpRegion.Offset = chain->PlayerSize;
      if (!DetectFormat(data, limit, chain->PlayerFP) || 
          !checker(data + chain->PlayerSize, limit - region.Offset))
      {
        continue;
      }
      if (Module::Holder::Ptr holder = creator(plugin, params, container, tmpRegion))
      {
        region = tmpRegion;
        return holder;
      }
    }
    return Module::Holder::Ptr();
  }
}

#endif //__CORE_PLUGINS_DETECT_HELPER_H_DEFINED__
