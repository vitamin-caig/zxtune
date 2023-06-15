/**
 *
 * @file
 *
 * @brief  YM/VTX chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/aym/aym_factory.h"
// library includes
#include <formats/chiptune/aym/ym.h>

namespace Module::YMVTX
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::YM::Decoder::Ptr decoder);
}  // namespace Module::YMVTX
