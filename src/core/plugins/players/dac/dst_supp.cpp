/**
* 
* @file
*
* @brief  DigitalStudio support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "dac_base.h"
#include "dac_plugin.h"
#include "digital.h"
#include "core/plugins/registrator.h"
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/digitalstudio.h>

namespace Module
{
namespace DigitalStudio
{
  const std::size_t CHANNELS_COUNT = 3;

  typedef DAC::ModuleData ModuleData;
  typedef DAC::DataBuilder DataBuilder;

  class Factory : public DAC::Factory
  {
  public:
    virtual DAC::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      const std::auto_ptr<DataBuilder> dataBuilder = DataBuilder::Create<CHANNELS_COUNT>(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::DigitalStudio::Parse(rawData, *dataBuilder))
      {
        propBuilder.SetSource(*container);
        return boost::make_shared<DAC::SimpleChiptune>(dataBuilder->GetResult(), propBuilder.GetResult(), CHANNELS_COUNT);
      }
      else
      {
        return DAC::Chiptune::Ptr();
      }
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

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateDigitalStudioDecoder();
    const Module::DAC::Factory::Ptr factory = boost::make_shared<Module::DigitalStudio::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
