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
#include <iterator.h>

namespace ZXTune
{
  struct DetectFormatChain
  {
    //no default ctor to declare as a structure
    const std::string PlayerFP;
    const std::size_t PlayerSize;
  };
  
  class MultiCheckedPlayerPluginHelper : public PlayerPlugin
  {
  public:
    virtual bool Check(const IO::DataContainer& inputData) const;

    virtual Module::Holder::Ptr CreateModule(Parameters::Accessor::Ptr parameters,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const;
  protected:
    typedef RangeIterator<const DetectFormatChain*> DetectorIterator;
    virtual DetectorIterator GetDetectors() const = 0;
    virtual bool CheckData(const uint8_t* data, std::size_t size) const = 0;
    virtual Module::Holder::Ptr TryToCreateModule(Parameters::Accessor::Ptr parameters,
      const MetaContainer& container, ModuleRegion& region) const = 0;
  };
}

#endif //__CORE_PLUGINS_DETECT_HELPER_H_DEFINED__
