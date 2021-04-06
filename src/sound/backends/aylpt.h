/**
 *
 * @file
 *
 * @brief  AYLPT subsystem interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <l10n/markup.h>

namespace Sound::AyLpt
{
  constexpr const Char BACKEND_ID[] = "aylpt";
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("Real AY via LPT backend");
}  // namespace Sound::AyLpt
