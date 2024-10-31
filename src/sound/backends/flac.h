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

#include <l10n/markup.h>
#include <sound/backend_attrs.h>

#include <types.h>

namespace Sound::Flac
{
  constexpr const auto BACKEND_ID = "flac"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("FLAC support backend.");
}  // namespace Sound::Flac
