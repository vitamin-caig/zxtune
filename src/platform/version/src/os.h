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
#elif defined(__linux__)
      static const char OS[] = "linux";
#elif defined(__ANDROID__)
      static const char OS[] = "android";
#elif defined(__APPLE__) && defined(__MACH__)
      static const char OS[] = "macosx";
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
#else
      static const char OS[] = "unknown-platform";
#endif
    }
  }
}
