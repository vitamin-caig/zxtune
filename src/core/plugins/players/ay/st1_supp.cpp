/*
Abstract:
  ST1 modules playback support

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

namespace ST1
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'S', 'T', '1', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | SupportedAYMFormatConvertors;

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

    virtual Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, Binary::Container::Ptr data) const
    {
      const std::auto_ptr< ::SoundTracker::DataBuilder> dataBuilder = ::SoundTracker::CreateDataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SoundTracker::Parse(*data, *dataBuilder))
      {
        propBuilder.SetSource(container);
        const AYM::Chiptune::Ptr chiptune = ::SoundTracker::CreateChiptune(dataBuilder->GetResult(), propBuilder.GetResult());
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
  void RegisterST1Support(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSoundTrackerDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<ST1::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ST1::ID, decoder->GetDescription(), ST1::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
