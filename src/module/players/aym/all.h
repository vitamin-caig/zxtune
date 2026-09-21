/**
 *
 * @file
 *
 * @brief  AYM-based chiptune factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_chiptune.h"

#include "binary/container.h"
#include "parameters/container.h"

namespace Module::AYM
{
  Chiptune::Ptr CreateASCSoundMasterChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateASCSoundMaster0Chiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateAYCChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreatePSGChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateFastTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateGlobalTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateProSoundCreatorChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateProSoundMakerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateProTracker1Chiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateProTracker2Chiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateSQTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateSoundTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateSoundTrackerCompiledChiptune(const Binary::Container& rawData,
                                                   Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateSoundTracker3Chiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateSoundTrackerProChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateYMChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateYMPackedChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
  Chiptune::Ptr CreateVTXChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties);
}  // namespace Module::AYM