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
      static const char OS[] = 
#if defined(_WIN32)
        "windows"
#elif defined(__linux__)
        "linux"
#elif defined(__ANDROID__)
        "android"
#elif defined(__APPLE__) && defined(__MACH__)
        "macosx"
#elif defined(__FreeBSD__)
        "freebsd"
#elif defined(__NetBSD__)
        "netbsd"
#elif defined(__OpenBSD__)
        "openbsd"
#elif defined(__unix__)
        "unix"
#elif defined(__CYGWIN__)
        "cygwin"
#else
        "unknown-platform"
#endif
      ;
    }
  }
}
