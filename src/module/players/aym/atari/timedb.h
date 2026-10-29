/**
 *
 * @file
 *
 * @brief  SNDH song length database interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"

#include "types.h"

#include <optional>

namespace Module::Atari
{
  // idx is a 0-based subsong index
  std::optional<uint_t> GetSndhFrames(Binary::View module, uint_t idx);
}  // namespace Module::Atari
