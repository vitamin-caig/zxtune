/**
 *
 * @file
 *
 * @brief OS detection
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

namespace Platform
{
  namespace Version
  {
    namespace Details
    {
#if defined(_WIN32)
      static const char OS[] = "windows";
#elif defined(__ANDROID__)
      static const char OS[] = "android";
#elif defined(__linux__)
      static const char OS[] = "linux";
#elif defined(__APPLE__) && defined(__MACH__)
      static const char OS[] = "darwin";
#elif defined(__FreeBSD__)
      static const char OS[] = "freebsd";
#elif defined(__NetBSD__)
      static const char OS[] = "netbsd";
#elif defined(__OpenBSD__)
      static const char OS[] = "openbsd";
#elif defined(__unix__)
      static const char OS[] = "unix";
#elif defined(__CYGWIN__)
      static const char OS[] = "cygwin";
#elif defined(__HAIKU__)
      static const char OS[] = "haiku";
#else
      static const char OS[] = "unknown-platform";
#endif
    }  // namespace Details
  }    // namespace Version
}  // namespace Platform
