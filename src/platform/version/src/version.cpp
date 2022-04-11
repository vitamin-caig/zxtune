/**
 *
 * @file
 *
 * @brief Version API implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
// Used information from http://sourceforge.net/p/predef/wiki/Home/
#include "platform/version/src/arch.h"
#include "platform/version/src/os.h"
#include "platform/version/src/toolset.h"
// library includes
#include <platform/version/api.h>
#include <strings/format.h>

namespace Platform
{
  namespace Version
  {
    String GetProgramTitle()
    {
      return PROGRAM_NAME;
    }

    String GetProgramVersion()
    {
#define TOSTRING(a) #a
#define STR(a) TOSTRING(a)
      constexpr const char VERSION[] = STR(BUILD_VERSION);
      static_assert(strlen(VERSION) > 0, "Undefined version");
      return VERSION;
    }

    String GetBuildDate()
    {
      static const char DATE[] = __DATE__;
      return DATE;
    }

    String GetBuildPlatform()
    {
      const String os = Details::OS;
      const String toolset = Details::TOOLSET;
      // some business-logic
      if (os == "windows" && toolset == "mingw")
      {
        return toolset;
      }
      else
      {
        return os;
      }
    }

    String GetBuildArchitecture()
    {
      return Details::ARCH;
    }

    String GetBuildArchitectureVersion()
    {
      return Details::ARCH_VERSION;
    }

    String GetProgramVersionString()
    {
      // 1- program name, 2- program version, 3- build date 4- platform, 5- architecture, 6- architecture version
      constexpr const Char PROGRAM_VERSION_STRING[] = "%1% %2% %3% %4%-%5% %6%";
      return Strings::Format(PROGRAM_VERSION_STRING, GetProgramTitle(), GetProgramVersion(), GetBuildDate(),
                             GetBuildPlatform(), GetBuildArchitecture(), GetBuildArchitectureVersion());
    }
  }  // namespace Version
}  // namespace Platform
