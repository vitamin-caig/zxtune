/**
 *
 * @file
 *
 * @brief  DAC-based chiptunes factory declaration
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/dac/dac_chiptune.h"
#include "module/players/factory.h"

#include "binary/container.h"
#include "parameters/container.h"

namespace Module::DAC
{
  using Factory = Chiptune::Ptr (*)(const Binary::Container& rawData, Parameters::Container::Ptr properties);

  Module::Factory::Ptr CreateModuleFactory(Factory create);
}  // namespace Module::DAC
