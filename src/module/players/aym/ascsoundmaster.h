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

// local includes
#include "module/players/aym/aym_factory.h"
// library includes
#include <formats/chiptune/aym/ascsoundmaster.h>

namespace Module::ASCSoundMaster
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder);
}  // namespace Module::ASCSoundMaster
