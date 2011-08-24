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

  class CommonPlayerPlugin : public PlayerPlugin
  {
  public:
    CommonPlayerPlugin(Plugin::Ptr descr, ModulesFactory::Ptr factory)
      : Description(descr)
      , Factory(factory)
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      return DetectModuleInLocation(Factory, Description, inputData, callback);
    }
  private:
    const Plugin::Ptr Description;
    const ModulesFactory::Ptr Factory;
  };
}

namespace ZXTune
{
  DetectionResult::Ptr DetectModuleInLocation(ModulesFactory::Ptr factory, Plugin::Ptr plugin, DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const IO::DataContainer::Ptr data = inputData->GetData();
    const DataFormat::Ptr format = factory->GetFormat();
    if (!factory->Check(*data))
    {
      return DetectionResult::CreateUnmatched(format, data);
    }
    const Parameters::Accessor::Ptr moduleParams = callback.CreateModuleParameters(inputData);
    const Module::ModuleProperties::RWPtr properties = Module::ModuleProperties::Create(plugin, inputData);
    std::size_t usedSize = 0;
    if (Module::Holder::Ptr holder = factory->CreateModule(properties, moduleParams, data, usedSize))
    {
      callback.ProcessModule(inputData, holder);
      return DetectionResult::CreateMatched(usedSize);
    }
    return DetectionResult::CreateUnmatched(format, data);
  }

  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, const String& info, const String& version, uint_t caps,
    ModulesFactory::Ptr factory)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, info, version, caps);
    return PlayerPlugin::Ptr(new CommonPlayerPlugin(description, factory));
  }
}
