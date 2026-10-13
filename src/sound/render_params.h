/**
 *
 * @file
 *
 * @brief  Rendering parameters helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/loop.h"
#include "parameters/accessor.h"

namespace Sound
{
  Module::LoopParameters GetLoopParameters(const Parameters::Accessor& params);
  uint_t GetSoundFrequency(const Parameters::Accessor& params);
}  // namespace Sound
