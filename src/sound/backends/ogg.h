/**
 *
 * @file
 *
 * @brief  OGG backend interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <l10n/markup.h>

namespace Sound::Ogg
{
  constexpr const Char BACKEND_ID[] = "ogg";
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("OGG support backend");
}  // namespace Sound::Ogg
