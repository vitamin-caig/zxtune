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
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  Analysis::Result::Ptr DetectModuleInLocation(ModulesFactory::Ptr factory, const String& type, DataLocation::Ptr inputData, const Module::DetectCallback& callback)
  {
    const Binary::Container::Ptr data = inputData->GetData();
    const Binary::Format::Ptr format = factory->GetFormat();
    if (!factory->Check(*data))
    {
      return Analysis::CreateUnmatchedResult(format, data);
    }
    Module::PropertiesBuilder properties;
    properties.SetType(type);
    properties.SetLocation(*inputData);
    if (Module::Holder::Ptr holder = factory->CreateModule(properties, *data))
    {
      callback.ProcessModule(inputData, holder);
      Parameters::IntType usedSize = 0;
      properties.GetResult()->FindValue(Module::ATTR_SIZE, usedSize);
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

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Factory->GetFormat();
    }

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      return DetectModuleInLocation(Factory, Description->Id(), inputData, callback);
    }

    virtual Module::Holder::Ptr Open(const Binary::Container& data) const
    {
      if (!Factory->Check(data))
      {
        return Module::Holder::Ptr();
      }
      Module::PropertiesBuilder properties;
      properties.SetType(Description->Id());
      return Factory->CreateModule(properties, data);
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
