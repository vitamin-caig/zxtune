/**
 *
 * @file
 *
 * @brief  AYM-based modules factory declaration
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_chiptune.h"
#include "module/players/factory.h"

#include "binary/container.h"
#include "parameters/container.h"

namespace Module::AYM
{
  using ChiptuneCreator = Chiptune::Ptr (*)(const Binary::Container& data, Parameters::Container::Ptr properties);

  Module::Factory::Ptr CreateModuleFactory(ChiptuneCreator create);
}  // namespace Module::AYM