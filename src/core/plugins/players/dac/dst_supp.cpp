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
#include "core/plugins/players/creation_result.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/digitalstudio.h>

#define FILE_TAG 3226C730

namespace DigitalStudio
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const std::size_t CHANNELS_COUNT = 3;

  typedef DAC::ModuleData ModuleData;
  typedef DAC::DataBuilder DataBuilder;
}

namespace DST
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'D', 'S', 'T', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_3DAC | CAP_CONV_RAW;

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

    virtual Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, Binary::Container::Ptr rawData) const
    {
      const std::auto_ptr< ::DigitalStudio::DataBuilder> dataBuilder = ::DigitalStudio::DataBuilder::Create< ::DigitalStudio::CHANNELS_COUNT>(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::DigitalStudio::Parse(*rawData, *dataBuilder))
      {
        propBuilder.SetSource(*container);
        const DAC::Chiptune::Ptr chiptune = boost::make_shared<DAC::SimpleChiptune>(dataBuilder->GetResult(), propBuilder.GetResult(), ::DigitalStudio::CHANNELS_COUNT);
        return DAC::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterDSTSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateDigitalStudioDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<DST::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(DST::ID, decoder->GetDescription(), DST::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
