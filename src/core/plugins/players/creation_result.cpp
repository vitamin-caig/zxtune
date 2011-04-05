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

namespace
{
  using namespace ZXTune;

  class NotFoundDetectionResult : public DetectionResult
  {
  public:
    NotFoundDetectionResult(IO::DataContainer::Ptr rawData, DataFormat::Ptr format)
      : Data(rawData)
      , Format(format)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      return 0;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      return Format->Search(Data->Data(), Data->Size());
    }
  private:
    const IO::DataContainer::Ptr Data;
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  DetectionResult::Ptr DetectModuleInLocation(ModulesFactory::Ptr factory, PlayerPlugin::Ptr plugin, DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const IO::DataContainer::Ptr data = inputData->GetData();
    const DataFormat::Ptr format = factory->GetFormat();
    if (!plugin->Check(*data))
    {
      return boost::make_shared<NotFoundDetectionResult>(data, format);
    }
    const Parameters::Accessor::Ptr moduleParams = callback.CreateModuleParameters(*inputData);
    const Module::ModuleProperties::Ptr properties = Module::ModuleProperties::Create(plugin, inputData);
    std::size_t usedSize = 0;
    if (Module::Holder::Ptr holder = factory->CreateModule(properties, moduleParams, data, usedSize))
    {
      ThrowIfError(callback.ProcessModule(*inputData, holder));
      return DetectionResult::Create(usedSize, 0);
    }
    return boost::make_shared<NotFoundDetectionResult>(data->GetSubcontainer(1, data->Size() - 1), format);
  }
}
