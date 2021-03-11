/**
 *
 * @file
 *
 * @brief  OpenAL subsystem access functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <strings/array.h>

namespace Sound
{
  namespace OpenAl
  {
    Strings::Array EnumerateDevices();
  }
}  // namespace Sound
