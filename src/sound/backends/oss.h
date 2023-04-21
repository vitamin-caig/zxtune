/**
 *
 * @file
 *
 * @brief  OSS backend interface
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

namespace Sound::Oss
{
  constexpr const auto BACKEND_ID = "oss"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("OSS sound system backend");
}  // namespace Sound::Oss
