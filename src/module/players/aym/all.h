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

namespace Module::AYM
{
  class PropertiesHelper;

  Chiptune::Ptr CreateASCSoundMasterChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateASCSoundMaster0Chiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateAYCChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreatePSGChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateFastTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateGlobalTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateProSoundCreatorChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateProSoundMakerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateProTracker1Chiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateProTracker2Chiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateSQTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateSoundTrackerChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateSoundTrackerCompiledChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateSoundTracker3Chiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateSoundTrackerProChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateYMChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateYMPackedChiptune(const Binary::Container& rawData, PropertiesHelper& props);
  Chiptune::Ptr CreateVTXChiptune(const Binary::Container& rawData, PropertiesHelper& props);
}  // namespace Module::AYM