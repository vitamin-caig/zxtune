/**
 *
 * @file
 *
 * @brief  SDL backend interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <l10n/markup.h>
#include <sound/backend_attrs.h>

#include <types.h>

namespace Sound::Sdl
{
  constexpr const auto BACKEND_ID = "sdl"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("SDL support backend");
}  // namespace Sound::Sdl
