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
#include "core/plugins/players/plugin.h"
#include "core/src/callback.h"
#include <core/plugins/plugins_types.h>
//common includes
#include <make_ptr.h>
//library includes
#include <core/plugin_attrs.h>
#include <module/attributes.h>
#include <module/players/properties_helper.h>
//std includes
#include <utility>

namespace ZXTune
{
  class CommonPlayerPlugin : public PlayerPlugin
  {
  public:
    CommonPlayerPlugin(Plugin::Ptr descr, Formats::Chiptune::Decoder::Ptr decoder, Module::Factory::Ptr factory)
      : Description(std::move(descr))
      , Decoder(std::move(decoder))
      , Factory(std::move(factory))
    {
    }

    Plugin::Ptr GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData, Module::DetectCallback& callback) const override
    {
      auto data = inputData->GetData();
      if (Decoder->Check(*data))
      {
        auto properties = callback.CreateInitialProperties(inputData->GetPath()->AsString());
        Module::PropertiesHelper props(*properties);
        props.SetContainer(inputData->GetPluginsChain()->AsString());
        if (auto holder = Factory->CreateModule(params, *data, properties))
        {
          props.SetType(Description->Id());
          callback.ProcessModule(*inputData, *Description, std::move(holder));
          Parameters::IntType usedSize = 0;
          properties->FindValue(Module::ATTR_SIZE, usedSize);
          return Analysis::CreateMatchedResult(static_cast<std::size_t>(usedSize));
        }
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), std::move(data));
    }

    Module::Holder::Ptr TryOpen(const Parameters::Accessor& params, const Binary::Container& data, Parameters::Container::Ptr properties) const override
    {
      if (Decoder->Check(data))
      {
        if (auto result = Factory->CreateModule(params, data, properties))
        {
          Module::PropertiesHelper(*properties).SetType(Description->Id());
          return result;
        }
      }
      return {};
    }
  private:
    const Plugin::Ptr Description;
    const Formats::Chiptune::Decoder::Ptr Decoder;
    const Module::Factory::Ptr Factory;
  };

  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, uint_t caps,
    Formats::Chiptune::Decoder::Ptr decoder, Module::Factory::Ptr factory)
  {
    auto description = CreatePluginDescription(id, decoder->GetDescription(), caps | Capabilities::Category::MODULE);
    return MakePtr<CommonPlayerPlugin>(std::move(description), std::move(decoder), std::move(factory));
  }
}
