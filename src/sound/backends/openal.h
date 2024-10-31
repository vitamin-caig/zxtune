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

#include <l10n/markup.h>
#include <sound/backend_attrs.h>
#include <strings/array.h>

namespace Sound::OpenAl
{
  constexpr const auto BACKEND_ID = "openal"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("OpenAL backend");

  Strings::Array EnumerateDevices();
}  // namespace Sound::OpenAl
