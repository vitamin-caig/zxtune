/**
 *
 * @file
 *
 * @brief  PulseAudio backend interface
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

namespace Sound::PulseAudio
{
  constexpr const auto BACKEND_ID = "paudio"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("PulseAudio support backend");
}  // namespace Sound::PulseAudio
