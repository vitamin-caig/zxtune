/**
* 
* @file
*
* @brief  DAC-based player plugin factory
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "dac_plugin.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/plugin_attrs.h>
#include <module/players/dac/dac_base.h>
#include <module/players/dac/dac_parameters.h>
#include <sound/mixer_factory.h>
//std includes
#include <utility>

namespace Module
{
  template<unsigned Channels>
  Devices::DAC::Chip::Ptr CreateChip(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    typedef Sound::FixedChannelsMatrixMixer<Channels> MixerType;
    const typename MixerType::Ptr mixer = MixerType::Create();
    const Parameters::Accessor::Ptr pollParams = Sound::CreateMixerNotificationParameters(params, mixer);
    const Devices::DAC::ChipParameters::Ptr chipParams = Module::DAC::CreateChipParameters(pollParams);
    return Devices::DAC::CreateChip(chipParams, mixer, target);
  }

  Devices::DAC::Chip::Ptr CreateChip(unsigned channels, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target)
  {
    switch (channels)
    {
    case 3:
      return CreateChip<3>(params, target);
    case 4:
      return CreateChip<4>(params, target);
    default:
      return Devices::DAC::Chip::Ptr();
    };
  }

  class DACHolder : public Holder
  {
  public:
    explicit DACHolder(DAC::Chiptune::Ptr chiptune)
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
      const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
      const DAC::DataIterator::Ptr iterator = Tune->CreateDataIterator();
      const Devices::DAC::Chip::Ptr chip = CreateChip(Tune->GetInformation()->ChannelsCount(), params, target);
      Tune->GetSamples(chip);
      return DAC::CreateRenderer(renderParams, iterator, chip);
    }
  private:
    const DAC::Chiptune::Ptr Tune;
  };

  class DACFactory : public Factory
  {
  public:
    explicit DACFactory(DAC::Factory::Ptr delegate)
      : Delegate(std::move(delegate))
    {
    }

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data, Parameters::Container::Ptr properties) const override
    {
      if (const DAC::Chiptune::Ptr chiptune = Delegate->CreateChiptune(data, properties))
      {
        return MakePtr<DACHolder>(chiptune);
      }
      else
      {
        return Holder::Ptr();
      }
    }
  private:
    const DAC::Factory::Ptr Delegate;
  };
}

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::DAC::Factory::Ptr factory)
  {
    const Module::Factory::Ptr modFactory = MakePtr<Module::DACFactory>(factory);
    const uint_t caps = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;
    return CreatePlayerPlugin(id, caps, decoder, modFactory);
  }
}
