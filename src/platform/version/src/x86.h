/**
 *
 * @file
 *
 * @brief x86 architecture detection
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

namespace Platform::Version::Details
{
  static const char ARCH[] = "x86";
#if _M_IX86 == 600 || defined(__i686__)
  static const char ARCH_VERSION[] = "i686";
#elif _M_IX86 == 500 || defined(__i586__)
  static const char ARCH_VERSION[] = "i586";
#elif _M_IX86 == 400 || defined(__i486__)
  static const char ARCH_VERSION[] = "i486";
#elif _M_IX86 == 300 || defined(__i386__)
  static const char ARCH_VERSION[] = "i386";
#elif defined(_M_IX86)
  static const char ARCH_VERSION[] = "blend";
#else
#  error "Not an x86 architecture"
#endif
}  // namespace Platform::Version::Details
