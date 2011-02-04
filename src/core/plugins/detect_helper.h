/*
Abstract:
  Module detect helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_DETECT_HELPER_H_DEFINED__
#define __CORE_PLUGINS_DETECT_HELPER_H_DEFINED__

//local includes
#include "enumerator.h"
//common includes
#include <iterator.h>

namespace ZXTune
{
  struct DataPrefix
  {
    //no default ctor to declare as a structure
    const std::string PrefixDetectString;
    const std::size_t PrefixSize;
  };

  typedef RangeIterator<const DataPrefix*> DataPrefixIterator;

  class DataDetector
  {
  public:
    virtual ~DataDetector() {}

    virtual bool CheckData(const uint8_t* data, std::size_t size) const = 0;
    virtual DataPrefixIterator GetPrefixes() const = 0;
  };

  class ModuleDetector : public DataDetector
  {
  public:
    virtual Module::Holder::Ptr TryToCreateModule(Parameters::Accessor::Ptr parameters,
      const MetaContainer& container, ModuleRegion& region) const = 0;
  };

  class ArchiveDetector : public DataDetector
  {
  public:
    virtual IO::DataContainer::Ptr TryToExtractSubdata(const Parameters::Accessor& parameters,
      const IO::DataContainer& data, std::size_t& originalDataSize) const = 0;
  };

  bool CheckDataFormat(const DataDetector& detector, const IO::DataContainer& inputData);

  Module::Holder::Ptr CreateModuleFromData(const ModuleDetector& detector,
    Parameters::Accessor::Ptr parameters, const MetaContainer& container, ModuleRegion& region);

  IO::DataContainer::Ptr ExtractSubdataFromData(const ArchiveDetector& detector,
    const Parameters::Accessor& parameters, const IO::DataContainer& data, ModuleRegion& region);
}

#endif //__CORE_PLUGINS_DETECT_HELPER_H_DEFINED__
