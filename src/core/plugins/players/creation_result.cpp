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

  Analysis::Result::Ptr DetectModuleInLocation(ModulesFactory::Ptr factory, Plugin::Ptr plugin, DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const Binary::Container::Ptr data = inputData->GetData();
    const Binary::Format::Ptr format = factory->GetFormat();
    if (!factory->Check(*data))
    {
      return Analysis::CreateUnmatchedResult(format, data);
    }
    const Module::ModuleProperties::RWPtr properties = Module::ModuleProperties::Create(plugin, inputData);
    std::size_t usedSize = 0;
    if (Module::Holder::Ptr holder = factory->CreateModule(properties, data, usedSize))
    {
      callback.ProcessModule(inputData, holder);
      return Analysis::CreateMatchedResult(usedSize);
    }
    return Analysis::CreateUnmatchedResult(format, data);
  }

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

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
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
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, const String& info, uint_t caps,
    ModulesFactory::Ptr factory)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, info, caps);
    return PlayerPlugin::Ptr(new CommonPlayerPlugin(description, factory));
  }
}
