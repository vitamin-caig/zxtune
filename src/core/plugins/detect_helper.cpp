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
  bool MultiCheckedPlayerPluginHelper::Check(const IO::DataContainer& inputData) const
  {
    const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
    const std::size_t limit = inputData.Size();

    //detect without
    if (CheckData(data, limit))
    {
      return true;
    }
    for (DetectorIterator chain = GetDetectors(); chain; ++chain)
    {
      if (limit > chain->PlayerSize &&
          DetectFormat(data, limit, chain->PlayerFP) &&
          CheckData(data + chain->PlayerSize, limit - chain->PlayerSize))
      {
        return true;
      }
    }
    return false;
  }

  Module::Holder::Ptr MultiCheckedPlayerPluginHelper::CreateModule(Parameters::Accessor::Ptr parameters,
                                           const MetaContainer& container,
                                           ModuleRegion& region) const
  {
    const std::size_t limit(container.Data->Size());
    const uint8_t* const data(static_cast<const uint8_t*>(container.Data->Data()));

    ModuleRegion tmpRegion;
    //try to detect without player
    if (CheckData(data, limit))
    {
      if (Module::Holder::Ptr holder = TryToCreateModule(parameters, container, tmpRegion))
      {
        region = tmpRegion;
        return holder;
      }
    }
    for (DetectorIterator chain = GetDetectors(); chain; ++chain)
    {
      tmpRegion.Offset = chain->PlayerSize;
      if (!DetectFormat(data, limit, chain->PlayerFP) || 
          !CheckData(data + chain->PlayerSize, limit - region.Offset))
      {
        continue;
      }
      if (Module::Holder::Ptr holder = TryToCreateModule(parameters, container, tmpRegion))
      {
        region = tmpRegion;
        return holder;
      }
    }
    return Module::Holder::Ptr();
  }
}
