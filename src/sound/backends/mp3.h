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

#include <l10n/markup.h>
#include <sound/backend_attrs.h>

#include <types.h>

namespace Sound::Mp3
{
  constexpr const auto BACKEND_ID = "mp3"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("MP3 support backend");
}  // namespace Sound::Mp3
