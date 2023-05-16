/**
 *
 * @file
 *
 * @brief ARM architecture detection
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

namespace Platform::Version::Details
{
#if defined(__aarch64__)
  static const char ARCH[] = "aarch64";
// fpu type
#elif defined(__arm__) && !defined(__SOFTFP__)
  static const char ARCH[] = "armhf";
#else
  static const char ARCH[] = "arm";
#endif

#if _M_ARM == 4 || defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__)
  static const char ARCH_VERSION[] = "v4";
#elif _M_ARM == 5 || defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5E__) || defined(__ARM_ARCH_5T__)                   \
    || defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__)
  static const char ARCH_VERSION[] = "v5";
#elif _M_ARM == 6 || defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6T2__)                  \
    || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6M__)
  static const char ARCH_VERSION[] = "v6";
#elif _M_ARM == 7 || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__)                   \
    || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
  static const char ARCH_VERSION[] = "v7";
#elif _M_ARM == 8 || defined(__ARM_ARCH_8__) || defined(__ARM_ARCH_8A__)
  static const char ARCH_VERSION[] = "v8";
#else
  static const char ARCH_VERSION[] = "";
#endif
}  // namespace Platform::Version::Details
