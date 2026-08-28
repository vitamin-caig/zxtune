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
#include "parameters/container.h"

namespace Module::TFM
{
  Chiptune::Ptr CreateTFCChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateTFDChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateTFMMusicMaker05Chiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateTFMMusicMaker13Chiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
}  // namespace Module::TFM