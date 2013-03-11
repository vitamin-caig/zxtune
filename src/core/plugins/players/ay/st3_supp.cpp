/*
Abstract:
  ST3 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtracker.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
//library includes
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
#include <formats/chiptune/decoders.h>
//boost includes
#include <boost/make_shared.hpp>

namespace ST3
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'S', 'T', '3', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::SupportedAYMFormatConvertors;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual bool Check(const Binary::Container& data) const
    {
      return Decoder->Check(data);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      const ::SoundTracker::Track::ModuleData::RWPtr modData = ::SoundTracker::Track::ModuleData::Create();
      const std::auto_ptr<Formats::Chiptune::SoundTracker::Builder> dataBuilder = ::SoundTracker::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SoundTracker::ParseVersion3(*data, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const Module::AYM::Chiptune::Ptr chiptune = ::SoundTracker::CreateChiptune(modData, properties);
        return AYM::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterST3Support(PlayerPluginsRegistrator& registrator)
  {
    {
      const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSoundTracker3Decoder();
      const ModulesFactory::Ptr factory = boost::make_shared<ST3::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ST3::ID, decoder->GetDescription(), ST3::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
