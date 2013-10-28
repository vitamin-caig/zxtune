/*
Abstract:
  DAC player plugin factory implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dac_plugin.h"
#include "dac_base.h"
#include "dac_parameters.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>
#include <sound/mixer_factory.h>

namespace
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
}

namespace Module
{
  class DACHolder : public Holder
  {
  public:
    explicit DACHolder(DAC::Chiptune::Ptr chiptune)
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
      : Delegate(delegate)
    {
    }

    virtual Holder::Ptr CreateModule(PropertiesBuilder& properties, const Binary::Container& data) const
    {
      if (const DAC::Chiptune::Ptr chiptune = Delegate->CreateChiptune(properties, data))
      {
        return boost::make_shared<DACHolder>(chiptune);
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
    const Module::Factory::Ptr modFactory = boost::make_shared<Module::DACFactory>(factory);
    const uint_t caps = CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;
    return CreatePlayerPlugin(id, caps, decoder, modFactory);
  }
}
