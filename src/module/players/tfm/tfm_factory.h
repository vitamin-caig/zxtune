/**
 *
 * @file
 *
 * @brief  TFM-based modules factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"
#include "module/players/tfm/tfm_chiptune.h"

#include "binary/container.h"
#include "parameters/container.h"

namespace Module::TFM
{
  using Factory = Chiptune::Ptr (*)(const Binary::Container& rawData, Parameters::Container::Ptr properties);

  Module::Factory::Ptr CreateModuleFactory(Factory create);
}  // namespace Module::TFM
