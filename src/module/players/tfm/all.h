/**
 *
 * @file
 *
 * @brief  TFM-based chiptune factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/tfm/tfm_chiptune.h"

#include "binary/container.h"

namespace Module
{
  class PropertiesHelper;
}

namespace Module::TFM
{
  Chiptune::Ptr CreateTFCChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateTFDChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateTFMMusicMaker05Chiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateTFMMusicMaker13Chiptune(const Binary::Container& rawData, PropertiesHelper& props);
}  // namespace Module::TFM