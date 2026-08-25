/**
 *
 * @file
 *
 * @brief  Game Music Emu-based formats support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/factory.h"

namespace Module::GME
{
  MultitrackFactory::Ptr CreateNsfFactory();
  MultitrackFactory::Ptr CreateNsfeFactory();
  MultitrackFactory::Ptr CreateGbsFactory();
  MultitrackFactory::Ptr CreateKssxFactory();
  MultitrackFactory::Ptr CreateHesFactory();

  ExternalParsingFactory::Ptr CreateGymFactory();
  ExternalParsingFactory::Ptr CreateKssFactory();
}  // namespace Module::GME
