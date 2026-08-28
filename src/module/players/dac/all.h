/**
 *
 * @file
 *
 * @brief  DAC-based chiptune factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/dac/dac_chiptune.h"

#include "binary/container.h"
#include "parameters/container.h"

namespace Module::DAC
{
  Chiptune::Ptr CreateChipTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateDigitalMusicMakerChiptune(const Binary::Container& rawData,
                                                Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateDigitalStudioChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateExtremeTracker1Chiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateProDigiTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateSampleTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateSQDigitalTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
}  // namespace Module::DAC