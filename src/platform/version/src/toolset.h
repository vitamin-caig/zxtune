/**
 *
 * @file
 *
 * @brief Toolset detection
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

namespace Platform::Version::Details
{
  static const char TOOLSET[] =
#if defined(_MSC_VER)
      "msvs"
#elif defined(__MINGW32__)
      "mingw"
#elif defined(__clang__)
      "clang"
#elif defined(__GNUC__)
      "gnuc"
#else
      "unknown-toolset"
#endif
      ;
}  // namespace Platform::Version::Details
