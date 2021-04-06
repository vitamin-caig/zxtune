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

// common includes
#include <types.h>
// library includes
#include <l10n/markup.h>

namespace Sound::Sdl
{
  constexpr const Char BACKEND_ID[] = "sdl";
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("SDL support backend");
}  // namespace Sound::Sdl
