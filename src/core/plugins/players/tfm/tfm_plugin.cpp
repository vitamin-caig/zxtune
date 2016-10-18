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
#include "core/plugins/players/plugin.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/plugin_attrs.h>
#include <module/players/tfm/tfm_base.h>
#include <module/players/tfm/tfm_parameters.h>
//std includes
#include <utility>

namespace Module
{
  class TFMHolder : public Holder
  {
  public:
    explicit TFMHolder(TFM::Chiptune::Ptr chiptune)
      : Tune(std::move(chiptune))
    {
    }

    Information::Ptr GetModuleInformation() const override
    {
      return Tune->GetInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Tune->GetProperties();
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
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
      : Delegate(std::move(delegate))
    {
    }

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data, Parameters::Container::Ptr properties) const override
    {
      if (const TFM::Chiptune::Ptr chiptune = Delegate->CreateChiptune(data, properties))
      {
        return MakePtr<TFMHolder>(chiptune);
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
    const Module::Factory::Ptr modFactory = MakePtr<Module::TFMFactory>(factory);
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
