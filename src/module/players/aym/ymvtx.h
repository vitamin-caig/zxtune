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

#include "formats/chiptune/aym/ym.h"
#include "module/players/aym/aym_factory.h"

namespace Module::YMVTX
{
  AYM::Factory::Ptr CreateFactory(Formats::Chiptune::YM::Decoder::Ptr decoder);
}  // namespace Module::YMVTX
