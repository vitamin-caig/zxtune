/**
 *
 * @file
 *
 * @brief  Defenition of resampling-related functionality
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "sound/receiver.h"

#include "types.h"

namespace Sound
{
  Converter::Ptr CreateResampler(uint_t inFreq, uint_t outFreq);
}  // namespace Sound
