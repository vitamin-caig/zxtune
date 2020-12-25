/**
*
* @file
*
* @brief  Darwin implementation of tools
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <debug/log.h>
#include <platform/tools.h>
//platform includes
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <mach-o/dyld.h>

namespace
{
  const Debug::Stream Dbg("Platform::Darwin");
}

namespace
{
  String GetSharedLibraryName()
  {
    Dl_info info;
    if (::dladdr(reinterpret_cast<void*>(&GetSharedLibraryName), &info) &&
        info.dli_sname && info.dli_saddr)
    {
      String result(info.dli_fname);
      Dbg("Shared library name: %1%", result);
      return result;
    }
    else
    {
      return String();
    }
  }
  
  String GetExecutableName()
  {
    String result(100, '\0');
    for (uint32_t size = result.size(); ; )
    {
      if (::_NSGetExecutablePath(result.data(), &size) == 0)
      {
        result.resize(size);
        break;
      }
      result.resize(size);
    }
    Dbg("Executable name: %1%", result);
    return result;
  }
}

namespace Platform
{
  String GetCurrentImageFilename()
  {
    static const auto soName = GetSharedLibraryName();
    if (!soName.empty())
    {
      Dbg("Shared library name is '%1%'", soName);
      return soName;
    }
    static const auto binName = GetExecutableName();
    return binName;
  }
}
