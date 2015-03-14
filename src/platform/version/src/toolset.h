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

namespace Platform
{
  namespace Version
  {
    namespace Details
    {
      static const char TOOLSET[] = 
#if defined(_MSC_VER)
        "msvs"
#elif defined(__MINGW32__)
        "mingw"
#elif defined(__GNUC__)
        "gnuc"
#elif defined(__clang__)
        "clang"
#else
        "unknown-toolset"
#endif
      ;
    }
  }
}
