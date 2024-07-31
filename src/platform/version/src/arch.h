/**
 *
 * @file
 *
 * @brief Architecture detection
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#if defined(_M_IX86) || defined(__i386__)
#  include "x86.h"
#elif defined(_M_ARM) || defined(__arm__) || defined(__aarch64__)
#  include "arm.h"
#else
namespace Platform::Version::Details
{
#  if defined(_M_AMD64) || defined(__amd64__)
  static const char ARCH[] = "x86_64";
  static const char ARCH_VERSION[] = "";
#  elif defined(_M_IA64) || defined(__ia64__)
  static const char ARCH[] = "ia64";
  static const char ARCH_VERSION[] = "";
#  elif defined(_MIPSEL)
  static const char ARCH[] = "mipsel";
  static const char ARCH_VERSION[] = "";
#  elif defined(__powerpc64__)
  static const char ARCH[] = "ppc64";
  static const char ARCH_VERSION[] = "";
#  elif defined(_M_PPC) || defined(__powerpc__)
  static const char ARCH[] = "ppc";
  static const char ARCH_VERSION[] = "";
#  elif defined(__loongarch64)
  static const char ARCH[] = "loongarch64";
  static const char ARCH_VERSION[] = __loongarch_arch;
#  else
  static const char ARCH[] = "unknown-arch";
  static const char ARCH_VERSION[] = "";
#  endif
}  // namespace Platform::Version::Details
#endif