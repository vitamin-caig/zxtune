/**
 *
 * @file
 *
 * @brief  FLAC backend interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <l10n/markup.h>

namespace Sound::Flac
{
  constexpr const Char BACKEND_ID[] = "flac";
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("FLAC support backend.");
}  // namespace Sound::Flac
