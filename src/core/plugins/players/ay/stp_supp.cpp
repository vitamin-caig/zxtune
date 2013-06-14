/*
Abstract:
  STP modules playback support

Last changed:
  $Id$

Author:
(C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtrackerpro.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
//library includes
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/aym/soundtrackerpro.h>
//boost includes
#include <boost/make_shared.hpp>

namespace STP
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'S', 'T', 'P', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | SupportedAYMFormatConvertors;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder)
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
      const std::auto_ptr< ::SoundTrackerPro::DataBuilder> dataBuilder = ::SoundTrackerPro::CreateDataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(*data, *dataBuilder))
      {
        propBuilder.SetSource(container);
        const AYM::Chiptune::Ptr chiptune = ::SoundTrackerPro::CreateChiptune(dataBuilder->GetResult(), propBuilder.GetResult());
        return AYM::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::SoundTrackerPro::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterSTPSupport(PlayerPluginsRegistrator& registrator)
  {
    {
      const Formats::Chiptune::SoundTrackerPro::Decoder::Ptr decoder = Formats::Chiptune::SoundTrackerPro::CreateCompiledModulesDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<STP::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(STP::ID, decoder->GetDescription(), STP::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
