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
#include <sound/backend_attrs.h>

namespace Sound::AyLpt
{
  constexpr const auto BACKEND_ID = "aylpt"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("Real AY via LPT backend");
}  // namespace Sound::AyLpt
