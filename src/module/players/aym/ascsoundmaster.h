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

#include "module/players/aym/aym_factory.h"
#include <formats/chiptune/aym/ascsoundmaster.h>

namespace Module::ASCSoundMaster
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder);
}  // namespace Module::ASCSoundMaster
