/**
 *
 * @file
 *
 * @brief  ProSoundMaker chiptune factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/aym/aym_factory.h"

namespace Module::ProSoundMaker
{
  AYM::Factory::Ptr CreateFactory();
}  // namespace Module::ProSoundMaker
