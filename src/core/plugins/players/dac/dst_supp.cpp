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
#include "dac_simple.h"
#include "core/plugins/player_plugins_registrator.h"
//library includes
#include <formats/chiptune/digital/digitalstudio.h>

namespace Module
{
namespace DigitalStudio
{
  const std::size_t CHANNELS_COUNT = 3;

  typedef DAC::SimpleModuleData ModuleData;
  typedef DAC::SimpleDataBuilder DataBuilder;

  class Factory : public DAC::Factory
  {
  public:
    virtual DAC::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const
    {
      DAC::PropertiesHelper props(*properties);
      const DataBuilder::Ptr dataBuilder = DAC::CreateSimpleDataBuilder<CHANNELS_COUNT>(props);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::DigitalStudio::Parse(rawData, *dataBuilder))
      {
        props.SetSource(*container);
        return DAC::CreateSimpleChiptune(dataBuilder->GetResult(), properties, CHANNELS_COUNT);
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
