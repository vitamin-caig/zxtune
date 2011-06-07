/*
Abstract:
  Archive extraction result implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "creation_result.h"
#include "core/src/callback.h"
//boost includes
#include <boost/make_shared.hpp>

namespace ZXTune
{
  DetectionResult::Ptr DetectModuleInLocation(ModulesFactory::Ptr factory, PlayerPlugin::Ptr plugin, DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const IO::DataContainer::Ptr data = inputData->GetData();
    const DataFormat::Ptr format = factory->GetFormat();
    if (!plugin->Check(*data))
    {
      return DetectionResult::CreateUnmatched(format, data);
    }
    const Parameters::Accessor::Ptr moduleParams = callback.CreateModuleParameters(*inputData);
    const Module::ModuleProperties::RWPtr properties = Module::ModuleProperties::Create(plugin, inputData);
    std::size_t usedSize = 0;
    if (Module::Holder::Ptr holder = factory->CreateModule(properties, moduleParams, data, usedSize))
    {
      ThrowIfError(callback.ProcessModule(*inputData, holder));
      return DetectionResult::CreateMatched(usedSize);
    }
    return DetectionResult::CreateUnmatched(format, data);
  }
}
