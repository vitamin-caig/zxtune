/**
 *
 * @file
 *
 * @brief  Multitrack archives filenames functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_type.h>
#include <string_view.h>
#include <types.h>
// std includes
#include <optional>

namespace Formats::Archived::MultitrackArchives
{
  String CreateFilename(std::size_t index);
  std::optional<std::size_t> ParseFilename(StringView str);
}  // namespace Formats::Archived::MultitrackArchives
