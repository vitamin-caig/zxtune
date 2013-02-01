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
#include <formats/chiptune/digitalstudio.h>

#define FILE_TAG 3226C730

namespace DST
{
  const std::size_t CHANNELS_COUNT = 3;

  //all samples has base freq at 8kHz (C-1)
  const uint_t BASE_FREQ = 8000;
}

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'D', 'S', 'T', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_3DAC | CAP_CONV_RAW;

  typedef Module::DAC::Digital<DST::CHANNELS_COUNT> DigitalStudio;

  class DSTModulesFactory : public ModulesFactory
  {
  public:
    explicit DSTModulesFactory(Formats::Chiptune::Decoder::Ptr decoder)
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
      const DigitalStudio::Track::ModuleData::RWPtr modData = DigitalStudio::Track::ModuleData::Create();
      DigitalStudio::Builder builder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::DigitalStudio::Parse(*data, builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        return boost::make_shared<DigitalStudio::Holder>(modData, properties, DST::BASE_FREQ);
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
    const ModulesFactory::Ptr factory = boost::make_shared<DSTModulesFactory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder->GetDescription(), CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
