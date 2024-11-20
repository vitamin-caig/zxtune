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

#include "l10n/markup.h"

#include "string_view.h"
#include "types.h"

namespace IO::Network
{
  constexpr auto PROVIDER_IDENTIFIER = "net"sv;
  constexpr auto PROVIDER_DESCRIPTION = L10n::translate("Network files access via different schemes support");
}  // namespace IO::Network
