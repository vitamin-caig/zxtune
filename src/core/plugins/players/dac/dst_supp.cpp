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

  const uint64_t Z80_FREQ = 3500000;
  //116+103+101+10=330 ticks/out cycle = 10606 outs/sec (AY)
  const uint_t TICKS_PER_CYCLE = 116 + 103 + 101 + 10;
  //C-1 step 88/256 32.7Hz = ~3645 samples/sec
  const uint_t C_1_STEP = 88;
  const uint_t BASE_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;

  typedef DAC::ModuleData ModuleData;
  typedef DAC::DataBuilder DataBuilder;
  typedef DAC::Digital<CHANNELS_COUNT>::Holder Holder;
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

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      const ::DigitalStudio::ModuleData::RWPtr modData = boost::make_shared< ::DigitalStudio::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::Digital::Builder> builder = ::DigitalStudio::DataBuilder::Create< ::DigitalStudio::CHANNELS_COUNT>(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::DigitalStudio::Parse(*data, *builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        return boost::make_shared< ::DigitalStudio::Holder>(modData, properties, ::DigitalStudio::BASE_FREQ);
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
