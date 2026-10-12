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

namespace Module::AYM
{
  class PropertiesHelper;

  using ChiptuneCreator = Chiptune::Ptr (*)(const Binary::Container& data, PropertiesHelper& props);

  Module::Factory::Ptr CreateModuleFactory(ChiptuneCreator create);
}  // namespace Module::AYM