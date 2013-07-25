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

namespace Module
{
  Analysis::Result::Ptr DetectInLocation(const Factory& factory, const String& type, ZXTune::DataLocation::Ptr inputData, const DetectCallback& callback)
  {
    const Binary::Container::Ptr data = inputData->GetData();
    const Binary::Format::Ptr format = factory.GetFormat();
    if (!factory.Check(*data))
    {
      return Analysis::CreateUnmatchedResult(format, data);
    }
    PropertiesBuilder properties;
    properties.SetType(type);
    properties.SetLocation(*inputData);
    if (const Holder::Ptr holder = factory.CreateModule(properties, *data))
    {
      callback.ProcessModule(inputData, holder);
      Parameters::IntType usedSize = 0;
      properties.GetResult()->FindValue(Module::ATTR_SIZE, usedSize);
      return Analysis::CreateMatchedResult(usedSize);
    }
    return Analysis::CreateUnmatchedResult(format, data);
  }
}

namespace ZXTune
{
  class CommonPlayerPlugin : public PlayerPlugin
  {
  public:
    CommonPlayerPlugin(Plugin::Ptr descr, Module::Factory::Ptr factory)
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
      return Module::DetectInLocation(*Factory, Description->Id(), inputData, callback);
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
    const Module::Factory::Ptr Factory;
  };
}

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, const String& info, uint_t caps,
    Module::Factory::Ptr factory)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, info, caps);
    return PlayerPlugin::Ptr(new CommonPlayerPlugin(description, factory));
  }
}
