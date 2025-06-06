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

#include "l10n/markup.h"
#include "sound/backend_attrs.h"

#include "types.h"

namespace Sound::Ogg
{
  constexpr const auto BACKEND_ID = "ogg"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("OGG support backend");
}  // namespace Sound::Ogg
