/**
 *
 * @file
 *
 * @brief  ASCSoundMaster chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/aym/ascsoundmaster.h"
#include "module/players/aym/aym_factory.h"

namespace Module::ASCSoundMaster
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::ASCSoundMaster::Parser parse);
}  // namespace Module::ASCSoundMaster
