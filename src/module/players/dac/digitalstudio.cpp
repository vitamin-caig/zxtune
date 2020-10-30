/**
* 
* @file
*
* @brief  DigitalStudio chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/dac/digitalstudio.h"
#include "module/players/dac/dac_simple.h"
//common includes
#include <make_ptr.h>
//library includes
#include <formats/chiptune/digital/digitalstudio.h>
//text includes
#include <module/text/platforms.h>

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
    DAC::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      DAC::PropertiesHelper props(*properties);
      DataBuilder::Ptr dataBuilder = DAC::CreateSimpleDataBuilder<CHANNELS_COUNT>(props);
      if (const auto container = Formats::Chiptune::DigitalStudio::Parse(rawData, *dataBuilder))
      {
        props.SetSource(*container);
        props.SetPlatform(Platforms::ZX_SPECTRUM);
        return DAC::CreateSimpleChiptune(dataBuilder->CaptureResult(), std::move(properties));
      }
      else
      {
        return DAC::Chiptune::Ptr();
      }
    }
  };
  
  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}
