/**
 *
 * @file
 *
 * @brief Version API implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// Used information from http://sourceforge.net/p/predef/wiki/Home/
#include "platform/version/src/arch.h"
#include "platform/version/src/os.h"
#include "platform/version/src/toolset.h"

#include "platform/version/api.h"
#include "strings/format.h"

#include "string_view.h"

namespace Platform::Version
{
  String GetProgramTitle()
  {
    return String{PROGRAM_NAME};
  }

  String GetProgramVersion()
  {
#define TOSTRING(a) #a
#define STR(a) TOSTRING(a)
    constexpr const char VERSION[] = STR(BUILD_VERSION);
    static_assert(VERSION[0] != 0, "Undefined version");
    return VERSION;
  }

  String GetBuildDate()
  {
    static const char DATE[] = __DATE__;
    return DATE;
  }

  String GetBuildPlatform()
  {
    // TODO: StringView
    String os = Details::OS;
    String toolset = Details::TOOLSET;
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
    constexpr auto PROGRAM_VERSION_STRING = "{} {} {} {}-{} {}"sv;
    return Strings::Format(PROGRAM_VERSION_STRING, GetProgramTitle(), GetProgramVersion(), GetBuildDate(),
                           GetBuildPlatform(), GetBuildArchitecture(), GetBuildArchitectureVersion());
  }
}  // namespace Platform::Version
