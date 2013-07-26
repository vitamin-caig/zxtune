/*
Abstract:
  DST modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dac_base.h"
#include "digital.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/digitalstudio.h>

#define FILE_TAG 3226C730

namespace Module
{
namespace DigitalStudio
{
  const std::size_t CHANNELS_COUNT = 3;

  typedef DAC::ModuleData ModuleData;
  typedef DAC::DataBuilder DataBuilder;

  class Factory : public Module::Factory
  {
  public:
    virtual Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      const std::auto_ptr<DataBuilder> dataBuilder = DataBuilder::Create<CHANNELS_COUNT>(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::DigitalStudio::Parse(rawData, *dataBuilder))
      {
        propBuilder.SetSource(*container);
        const DAC::Chiptune::Ptr chiptune = boost::make_shared<DAC::SimpleChiptune>(dataBuilder->GetResult(), propBuilder.GetResult(), CHANNELS_COUNT);
        return DAC::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterDSTSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'D', 'S', 'T', 0};
    const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_3DAC | CAP_CONV_RAW;

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateDigitalStudioDecoder();
    const Module::Factory::Ptr factory = boost::make_shared<Module::DigitalStudio::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
