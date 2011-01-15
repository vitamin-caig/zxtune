/*
Abstract:
  Module detect helper implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "detect_helper.h"
//common includes
#include <detector.h>

namespace ZXTune
{
  bool CheckDataFormat(const DataDetector& detector, const IO::DataContainer& inputData)
  {
    const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
    const std::size_t limit = inputData.Size();

    //detect without
    if (detector.CheckData(data, limit))
    {
      return true;
    }
    for (DataPrefixIterator chain = detector.GetPrefixes(); chain; ++chain)
    {
      if (limit > chain->PrefixSize &&
          DetectFormat(data, limit, chain->PrefixDetectString) &&
          detector.CheckData(data + chain->PrefixSize, limit - chain->PrefixSize))
      {
        return true;
      }
    }
    return false;
  }

  Module::Holder::Ptr CreateModuleFromData(const ModuleDetector& detector,
    Parameters::Accessor::Ptr parameters, const MetaContainer& container, ModuleRegion& region)
  {
    const std::size_t limit(container.Data->Size());
    const uint8_t* const data(static_cast<const uint8_t*>(container.Data->Data()));

    ModuleRegion tmpRegion;
    //try to detect without player
    if (detector.CheckData(data, limit))
    {
      if (Module::Holder::Ptr holder = detector.TryToCreateModule(parameters, container, tmpRegion))
      {
        region = tmpRegion;
        return holder;
      }
    }
    for (DataPrefixIterator chain = detector.GetPrefixes(); chain; ++chain)
    {
      tmpRegion.Offset = chain->PrefixSize;
      if (limit < chain->PrefixSize ||
          !DetectFormat(data, limit, chain->PrefixDetectString) ||
          !detector.CheckData(data + chain->PrefixSize, limit - region.Offset))
      {
        continue;
      }
      if (Module::Holder::Ptr holder = detector.TryToCreateModule(parameters, container, tmpRegion))
      {
        region = tmpRegion;
        return holder;
      }
    }
    return Module::Holder::Ptr();
  }

  IO::DataContainer::Ptr ExtractSubdataFromData(const ArchiveDetector& detector,
    const Parameters::Accessor& parameters, const MetaContainer& container, ModuleRegion& region)
  {
    const std::size_t limit(container.Data->Size());
    const uint8_t* const data(static_cast<const uint8_t*>(container.Data->Data()));

    ModuleRegion tmpRegion;
    //try to detect without player
    if (detector.CheckData(data, limit))
    {
      if (IO::DataContainer::Ptr subdata = detector.TryToExtractSubdata(parameters, container, tmpRegion))
      {
        region = tmpRegion;
        return subdata;
      }
    }
    for (DataPrefixIterator chain = detector.GetPrefixes(); chain; ++chain)
    {
      tmpRegion.Offset = chain->PrefixSize;
      if (limit < chain->PrefixSize ||
          !DetectFormat(data, limit, chain->PrefixDetectString) ||
          !detector.CheckData(data + chain->PrefixSize, limit - region.Offset))
      {
        continue;
      }
      if (IO::DataContainer::Ptr subdata = detector.TryToExtractSubdata(parameters, container, tmpRegion))
      {
        region = tmpRegion;
        return subdata;
      }
    }
    return IO::DataContainer::Ptr();
  }
}
