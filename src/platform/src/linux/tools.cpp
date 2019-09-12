/**
*
* @file
*
* @brief  Linux implementation of tools
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
#include <sys/stat.h>

namespace
{
  const Debug::Stream Dbg("Platform::Linux");
}

namespace
{
  std::string GetSharedLibraryName()
  {
    Dl_info info;
    if (::dladdr(reinterpret_cast<void*>(&GetSharedLibraryName), &info) &&
        info.dli_sname && info.dli_saddr)
    {
      const std::string result(info.dli_fname);
      Dbg("Shared library name: %1%", result);
      return result;
    }
    else
    {
      return std::string();
    }
  }
  
  std::string GetExecutableName()
  {
    const std::string selfPath = "/proc/self/exe";
    struct stat sb;
    if (-1 == ::lstat(selfPath.c_str(), &sb))
    {
      Dbg("Failed to stat %1% (errno %2%)", selfPath, errno);
      return std::string();
    }
    if (!S_ISLNK(sb.st_mode))
    {
      Dbg("%1% is not a symlink", selfPath);
      return std::string();
    }
    
    std::vector<char> filename(1024);
    for (;;)
    {
      const int len = ::readlink(selfPath.c_str(), filename.data(), filename.size() - 1);
      if (len == -1)
      {
        Dbg("Failed to readlink '%1%' (errno %2%)", selfPath, errno);
        return std::string();
      }
      else if (len == static_cast<int>(filename.size()) - 1)
      {
        filename.resize(filename.size() * 2);
      }
      else
      {
        filename[len] = 0;
        break;
      }
    }
    const std::string result(filename.data());
    Dbg("Executable name: %1%", result);
    return result;
  }
}

namespace Platform
{
  std::string GetCurrentImageFilename()
  {
    static const std::string soName = GetSharedLibraryName();
    if (!soName.empty())
    {
      Dbg("Shared library name is '%1%'", soName);
      return soName;
    }
    static const std::string binName = GetExecutableName();
    return binName;
  }
}
