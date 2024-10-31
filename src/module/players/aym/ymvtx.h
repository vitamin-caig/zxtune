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

#include "module/players/aym/aym_factory.h"
#include <formats/chiptune/aym/ym.h>

namespace Module::YMVTX
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::YM::Decoder::Ptr decoder);
}  // namespace Module::YMVTX
