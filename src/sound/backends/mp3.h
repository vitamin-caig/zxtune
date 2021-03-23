/**
 *
 * @file
 *
 * @brief  MP3 backend interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <l10n/markup.h>

namespace Sound::Mp3
{
  constexpr const Char BACKEND_ID[] = "mp3";
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("MP3 support backend");
}  // namespace Sound::Mp3
