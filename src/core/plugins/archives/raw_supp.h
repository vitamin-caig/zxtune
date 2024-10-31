/**
 *
 * @file
 *
 * @brief  Raw scanner support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_type.h"

#include <cstddef>

namespace ZXTune::Raw
{
  String CreateFilename(std::size_t offset);
}
