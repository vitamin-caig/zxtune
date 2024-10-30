/**
 *
 * @file
 *
 * @brief  std::filesystem adapters
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_type.h>
#include <string_view.h>
// std includes
#include <filesystem>

namespace IO::Details
{
  inline String ToString(const std::filesystem::path& path)
  {
    const auto u8string = path.u8string();
    return {u8string.begin(), u8string.end()};
  }

  inline std::filesystem::path FromString(StringView str)
  {
    return std::filesystem::u8path(str.begin(), str.end());
  }
}  // namespace IO::Details
