/*
Abstract:
  DST modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "digital.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/sampletracker.h>

#define FILE_TAG ADBE77A4

namespace SampleTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const std::size_t CHANNELS_COUNT = 3;

  typedef DAC::ModuleData ModuleData;
  typedef DAC::DataBuilder DataBuilder;

  /*
    0x0016 0x0017 0x0019 0x001a 0x001c 0x001e 0x001f 0x0021 0x0023 0x0025 0x0027 0x002a
    0x002c 0x002f 0x0032 0x0035 0x0038 0x003b 0x003f 0x0042 0x0046 0x004b 0x004f 0x0054
    0x0059 0x005e 0x0064 0x0069 0x0070 0x0076 0x007d 0x0085 0x008d 0x0095 0x009e 0x00a7
    0x00b1 0x00bc 0x00c7 0x00d3 0x00df 0x00ed 0x00fb 0x010a 0x0119 0x012a 0x013c 0x014f
    0x0163 0x0178 0x018e 0x01a6 0x01bf 0x01d9 0x01f6 0x0213 0x0233 0x0254 0x0278 0x029d
    0x02c5 0x02ef 0x031c 0x034b 0x0373 0x03b3 0x03eb 0x0427 0x0466 0x04a9 0x04f0 0x053b
  */
}

namespace STR
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'S', 'T', 'R', 0};
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

    virtual Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      const std::auto_ptr< ::SampleTracker::DataBuilder> dataBuilder = ::SampleTracker::DataBuilder::Create< ::SampleTracker::CHANNELS_COUNT>(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SampleTracker::Parse(rawData, *dataBuilder))
      {
        propBuilder.SetSource(*container);
        const DAC::Chiptune::Ptr chiptune = boost::make_shared<DAC::SimpleChiptune>(dataBuilder->GetResult(), propBuilder.GetResult(), ::SampleTracker::CHANNELS_COUNT);
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
  void RegisterSTRSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateSampleTrackerDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<STR::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(STR::ID, decoder->GetDescription(), STR::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
