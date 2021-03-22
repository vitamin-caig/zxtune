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
    constexpr const Char BACKEND_ID[] = "openal";

    Strings::Array EnumerateDevices();
  }  // namespace OpenAl
}  // namespace Sound
