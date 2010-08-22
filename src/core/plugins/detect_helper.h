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

  inline bool PerformDetect(
    const boost::function<bool(const uint8_t*, std::size_t, const MetaContainer&, Module::Holder::Ptr&, ModuleRegion&)>& checker,
    const DetectFormatChain* chainBegin, const DetectFormatChain* chainEnd,
    const MetaContainer& container, Module::Holder::Ptr& holder, ModuleRegion& region)
  {
    const std::size_t limit(container.Data->Size());
    const uint8_t* const data(static_cast<const uint8_t*>(container.Data->Data()));

    ModuleRegion tmpRegion;
    //try to detect without player
    if (checker(data, limit, container, holder, tmpRegion))
    {
      region = tmpRegion;
      return true;
    }
    for (const DetectFormatChain* chain = chainBegin; chain != chainEnd; ++chain)
    {
      tmpRegion.Offset = chain->PlayerSize;
      if (DetectFormat(data, limit, chain->PlayerFP) &&
          checker(data + chain->PlayerSize, limit - region.Offset, container, holder, tmpRegion))
      {
        region = tmpRegion;
        return true;
      }
    }
    return false;
  }
}

#endif //__CORE_PLUGINS_DETECT_HELPER_H_DEFINED__
