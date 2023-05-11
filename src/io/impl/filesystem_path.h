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
#include <types.h>
// std includes
#include <filesystem>

namespace IO
{
  namespace Details
  {
    inline String ToString(const std::filesystem::path& path)
    {
      return path.u8string();
    }

    inline std::filesystem::path FromString(StringView str)
    {
      return std::filesystem::u8path(str.begin(), str.end());
    }
  }  // namespace Details
}  // namespace IO
