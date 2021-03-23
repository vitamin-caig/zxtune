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
#include <l10n/markup.h>
#include <strings/array.h>

namespace Sound
{
  namespace OpenAl
  {
    constexpr const Char BACKEND_ID[] = "openal";
    constexpr auto BACKEND_DESCRIPTION = L10n::translate("OpenAL backend");

    Strings::Array EnumerateDevices();
  }  // namespace OpenAl
}  // namespace Sound
