/**
 *
 * @file
 *
 * @brief  Linux implementation of tools
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <debug/log.h>
#include <platform/tools.h>

#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <vector>

namespace
{
  const Debug::Stream Dbg("Platform::Linux");
}

namespace
{
  String GetSharedLibraryName()
  {
    Dl_info info;
    if (::dladdr(reinterpret_cast<void*>(&GetSharedLibraryName), &info) && info.dli_sname && info.dli_saddr)
    {
      String result(info.dli_fname);
      Dbg("Shared library name: {}", result);
      return result;
    }
    else
    {
      return {};
    }
  }

  String GetExecutableName()
  {
    const String selfPath = "/proc/self/exe";
    struct stat sb;
    if (-1 == ::lstat(selfPath.c_str(), &sb))
    {
      Dbg("Failed to stat {} (errno {})", selfPath, errno);
      return {};
    }
    if (!S_ISLNK(sb.st_mode))
    {
      Dbg("{} is not a symlink", selfPath);
      return {};
    }

    std::vector<char> filename(1024);
    for (;;)
    {
      const int len = ::readlink(selfPath.c_str(), filename.data(), filename.size() - 1);
      if (len == -1)
      {
        Dbg("Failed to readlink '{}' (errno {})", selfPath, errno);
        return {};
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
    String result(filename.data());
    Dbg("Executable name: {}", result);
    return result;
  }
}  // namespace

namespace Platform
{
  String GetCurrentImageFilename()
  {
    static const String soName = GetSharedLibraryName();
    if (!soName.empty())
    {
      Dbg("Shared library name is '{}'", soName);
      return soName;
    }
    static const String binName = GetExecutableName();
    return binName;
  }
}  // namespace Platform
