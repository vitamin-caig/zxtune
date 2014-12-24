/**
* 
* @file
*
* @brief  MIDI-based player plugin factory
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "midi_plugin.h"
#include "midi_base.h"
#include "midi_parameters.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>

namespace Module
{
  class MIDIHolder : public Holder
  {
  public:
    explicit MIDIHolder(MIDI::Chiptune::Ptr chiptune)
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
      const Devices::MIDI::ChipParameters::Ptr chipParams = MIDI::CreateChipParameters(params);
      const Devices::MIDI::Chip::Ptr chip = Devices::MIDI::CreateChip(chipParams, target);
      const Sound::RenderParameters::Ptr soundParams = Sound::RenderParameters::Create(params);
      const MIDI::DataIterator::Ptr iterator = Tune->CreateDataIterator();
      return MIDI::CreateRenderer(soundParams, iterator, chip);
    }
  private:
    const MIDI::Chiptune::Ptr Tune;
  };

  class MIDIFactory : public Factory
  {
  public:
    explicit MIDIFactory(MIDI::Factory::Ptr delegate)
      : Delegate(delegate)
    {
    }

    virtual Holder::Ptr CreateModule(PropertiesBuilder& properties, const Binary::Container& data) const
    {
      if (const MIDI::Chiptune::Ptr chiptune = Delegate->CreateChiptune(properties, data))
      {
        return boost::make_shared<MIDIHolder>(chiptune);
      }
      else
      {
        return Holder::Ptr();
      }
    }
  private:
    const MIDI::Factory::Ptr Delegate;
  };
}

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::MIDI::Factory::Ptr factory)
  {
    const Module::Factory::Ptr modFactory = boost::make_shared<Module::MIDIFactory>(factory);
    const uint_t caps = CAP_STOR_MODULE | CAP_DEV_MIDI | CAP_CONV_RAW;
    return CreatePlayerPlugin(id, caps, decoder, modFactory);
  }
}
