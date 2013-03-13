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
#include <formats/chiptune/sampletracker.h>

#define FILE_TAG ADBE77A4

namespace SampleTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const std::size_t CHANNELS_COUNT = 3;

  //all samples has base freq at 2kHz (C-1)
  const uint_t BASE_FREQ = 2000;

  typedef DAC::ModuleData ModuleData;
  typedef DAC::DataBuilder DataBuilder;
  typedef DAC::Digital<CHANNELS_COUNT>::Holder Holder;
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

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      const ::SampleTracker::ModuleData::RWPtr modData = boost::make_shared< ::SampleTracker::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::Digital::Builder> builder = ::SampleTracker::DataBuilder::Create< ::SampleTracker::CHANNELS_COUNT>(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::SampleTracker::Parse(*data, *builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        return boost::make_shared<SampleTracker::Holder>(modData, properties, ::SampleTracker::BASE_FREQ);
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
