/**
* 
* @file
*
* @brief  Player plugin implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "plugin.h"
#include "core/src/callback.h"
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/make_shared.hpp>

namespace ZXTune
{
  class CommonPlayerPlugin : public PlayerPlugin
  {
  public:
    CommonPlayerPlugin(Plugin::Ptr descr, Formats::Chiptune::Decoder::Ptr decoder, Module::Factory::Ptr factory)
      : Description(descr)
      , Decoder(decoder)
      , Factory(factory)
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const Binary::Container::Ptr data = inputData->GetData();
      if (Decoder->Check(*data))
      {
        Module::PropertiesBuilder properties;
        properties.SetType(Description->Id());
        properties.SetLocation(*inputData);
        if (const Module::Holder::Ptr holder = Factory->CreateModule(properties, *data))
        {
          callback.ProcessModule(inputData, holder);
          Parameters::IntType usedSize = 0;
          properties.GetResult()->FindValue(Module::ATTR_SIZE, usedSize);
          return Analysis::CreateMatchedResult(static_cast<std::size_t>(usedSize));
        }
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), data);
    }

    virtual Module::Holder::Ptr Open(const Binary::Container& data) const
    {
      if (Decoder->Check(data))
      {
        Module::PropertiesBuilder properties;
        properties.SetType(Description->Id());
        return Factory->CreateModule(properties, data);
      }
      return Module::Holder::Ptr();
    }
  private:
    const Plugin::Ptr Description;
    const Formats::Chiptune::Decoder::Ptr Decoder;
    const Module::Factory::Ptr Factory;
  };

  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, uint_t caps,
    Formats::Chiptune::Decoder::Ptr decoder, Module::Factory::Ptr factory)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, decoder->GetDescription(), caps);
    return PlayerPlugin::Ptr(new CommonPlayerPlugin(description, decoder, factory));
  }
}
