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
#include <sound/backend_attrs.h>

namespace Sound::Mp3
{
  constexpr const auto BACKEND_ID = "mp3"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("MP3 support backend");
}  // namespace Sound::Mp3
