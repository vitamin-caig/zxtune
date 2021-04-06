/**
 *
 * @file
 *
 * @brief  Network provider-related interfaces
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <l10n/markup.h>

namespace IO::Network
{
  constexpr const Char PROVIDER_IDENTIFIER[] = "net";
  constexpr auto PROVIDER_DESCRIPTION = L10n::translate("Network files access via different schemes support");
}  // namespace IO::Network
