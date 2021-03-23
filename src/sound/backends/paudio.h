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

namespace Sound::PulseAudio
{
  constexpr const Char BACKEND_ID[] = "paudio";
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("PulseAudio support backend");
}  // namespace Sound::PulseAudio
