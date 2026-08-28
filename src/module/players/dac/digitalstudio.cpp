/**
 *
 * @file
 *
 * @brief  DigitalStudio chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/digitalstudio.h"

#include "module/players/dac/dac_simple.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "make_ptr.h"

namespace Module::DigitalStudio
{
  const std::size_t CHANNELS_COUNT = 3;

  using ModuleData = DAC::SimpleModuleData;
  using DataBuilder = DAC::SimpleDataBuilder;

}  // namespace Module::DigitalStudio

namespace Module::DAC
{
  Chiptune::Ptr CreateDigitalStudioChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties)
  {
    PropertiesHelper props(*properties, DigitalStudio::CHANNELS_COUNT);
    DigitalStudio::DataBuilder::Ptr dataBuilder = CreateSimpleDataBuilder<DigitalStudio::CHANNELS_COUNT>(props);
    if (const auto container = Formats::Chiptune::DigitalStudio::Parse(rawData, *dataBuilder))
    {
      props.SetSource(*container);
      return CreateSimpleChiptune(dataBuilder->CaptureResult(), std::move(properties));
    }
    return {};
  }
}  // namespace Module::DAC
