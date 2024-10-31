/**
 *
 * @file
 *
 * @brief  Player plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/plugin.h"

#include "module/players/properties_helper.h"

#include "analysis/path.h"
#include "analysis/result.h"
#include "binary/format.h"
#include "core/data_location.h"
#include "core/module_detect.h"
#include "core/plugin_attrs.h"
#include "module/attributes.h"
#include "module/holder.h"
#include "parameters/container.h"

#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"

#include <memory>
#include <utility>

namespace ZXTune
{
  class CommonPlayerPlugin : public PlayerPlugin
  {
  public:
    CommonPlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder, Module::Factory::Ptr factory)
      : Identifier(id)
      , Caps(caps)
      , Decoder(std::move(decoder))
      , Factory(std::move(factory))
    {}

    PluginId Id() const override
    {
      return Identifier;
    }

    StringView Description() const override
    {
      return Decoder->GetDescription();
    }

    uint_t Capabilities() const override
    {
      return Caps;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 Module::DetectCallback& callback) const override
    {
      auto data = inputData->GetData();
      if (Decoder->Check(*data))
      {
        auto properties = callback.CreateInitialProperties(inputData->GetPath()->AsString());
        Module::PropertiesHelper props(*properties);
        props.SetContainer(inputData->GetPluginsChain()->AsString());
        if (auto holder = Factory->CreateModule(params, *data, properties))
        {
          props.SetType(Identifier);
          callback.ProcessModule(*inputData, *this, std::move(holder));
          const auto usedSize = Parameters::GetInteger<std::size_t>(*properties, Module::ATTR_SIZE);
          return Analysis::CreateMatchedResult(usedSize);
        }
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), std::move(data));
    }

    Module::Holder::Ptr TryOpen(const Parameters::Accessor& params, const Binary::Container& data,
                                Parameters::Container::Ptr properties) const override
    {
      if (Decoder->Check(data))
      {
        if (auto result = Factory->CreateModule(params, data, properties))
        {
          Module::PropertiesHelper(*properties).SetType(Identifier);
          return result;
        }
      }
      return {};
    }

  private:
    const PluginId Identifier;
    const uint_t Caps;
    const Formats::Chiptune::Decoder::Ptr Decoder;
    const Module::Factory::Ptr Factory;
  };

  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::Factory::Ptr factory)
  {
    return MakePtr<CommonPlayerPlugin>(id, caps | Capabilities::Category::MODULE, std::move(decoder),
                                       std::move(factory));
  }

  class ExternalParsingPlayerPlugin : public PlayerPlugin
  {
  public:
    ExternalParsingPlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                Module::ExternalParsingFactory::Ptr factory)
      : Identifier(id)
      , Caps(caps)
      , Decoder(std::move(decoder))
      , Factory(std::move(factory))
    {}

    PluginId Id() const override
    {
      return Identifier;
    }

    StringView Description() const override
    {
      return Decoder->GetDescription();
    }

    uint_t Capabilities() const override
    {
      return Caps;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 Module::DetectCallback& callback) const override
    {
      auto data = inputData->GetData();
      if (const auto container = Decoder->Decode(*data))
      {
        auto properties = callback.CreateInitialProperties(inputData->GetPath()->AsString());
        Module::PropertiesHelper props(*properties);
        props.SetContainer(inputData->GetPluginsChain()->AsString());
        if (auto holder = Factory->CreateModule(params, *container, properties))
        {
          props.SetSource(*container);
          props.SetType(Identifier);
          callback.ProcessModule(*inputData, *this, std::move(holder));
          return Analysis::CreateMatchedResult(container->Size());
        }
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), std::move(data));
    }

    Module::Holder::Ptr TryOpen(const Parameters::Accessor& params, const Binary::Container& data,
                                Parameters::Container::Ptr properties) const override
    {
      if (const auto container = Decoder->Decode(data))
      {
        if (auto result = Factory->CreateModule(params, *container, properties))
        {
          Module::PropertiesHelper props(*properties);
          props.SetSource(*container);
          props.SetType(Identifier);
          return result;
        }
      }
      return {};
    }

  private:
    const PluginId Identifier;
    const uint_t Caps;
    const Formats::Chiptune::Decoder::Ptr Decoder;
    const Module::ExternalParsingFactory::Ptr Factory;
  };

  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::ExternalParsingFactory::Ptr factory)
  {
    return MakePtr<ExternalParsingPlayerPlugin>(id, caps | Capabilities::Category::MODULE, std::move(decoder),
                                                std::move(factory));
  }
}  // namespace ZXTune
