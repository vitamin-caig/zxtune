/**
* 
* @file
*
* @brief  TFM-based player plugin factory
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tfm_plugin.h"
#include "tfm_base.h"
#include "tfm_parameters.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>

namespace Module
{
  class TFMHolder : public Holder
  {
  public:
    explicit TFMHolder(TFM::Chiptune::Ptr chiptune)
      : Tune(chiptune)
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Tune->GetInformation();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Tune->GetProperties();
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      const Devices::TFM::ChipParameters::Ptr chipParams = TFM::CreateChipParameters(params);
      const Devices::TFM::Chip::Ptr chip = Devices::TFM::CreateChip(chipParams, target);
      const Sound::RenderParameters::Ptr soundParams = Sound::RenderParameters::Create(params);
      const TFM::DataIterator::Ptr iterator = Tune->CreateDataIterator();
      return TFM::CreateRenderer(soundParams, iterator, chip);
    }
  private:
    const TFM::Chiptune::Ptr Tune;
  };

  class TFMFactory : public Factory
  {
  public:
    explicit TFMFactory(TFM::Factory::Ptr delegate)
      : Delegate(delegate)
    {
    }

    virtual Holder::Ptr CreateModule(PropertiesBuilder& properties, const Binary::Container& data) const
    {
      if (const TFM::Chiptune::Ptr chiptune = Delegate->CreateChiptune(properties, data))
      {
        return boost::make_shared<TFMHolder>(chiptune);
      }
      else
      {
        return Holder::Ptr();
      }
    }
  private:
    const TFM::Factory::Ptr Delegate;
  };
}

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder, Module::TFM::Factory::Ptr factory)
  {
    const Module::Factory::Ptr modFactory = boost::make_shared<Module::TFMFactory>(factory);
    const uint_t tfmCaps = Capabilities::Module::Device::TURBOFM;
    return CreatePlayerPlugin(id, caps | tfmCaps, decoder, modFactory);
  }

  PlayerPlugin::Ptr CreateTrackPlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::TFM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::TRACK, decoder, factory);
  }
  
  PlayerPlugin::Ptr CreateStreamPlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::TFM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::STREAM, decoder, factory);
  }
}
