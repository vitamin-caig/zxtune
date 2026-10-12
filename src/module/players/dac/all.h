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

namespace Module::DAC
{
  class PropertiesHelper;

  Chiptune::Ptr CreateChipTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateDigitalMusicMakerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateDigitalStudioChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateExtremeTracker1Chiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateProDigiTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateSampleTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateSQDigitalTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
}  // namespace Module::DAC