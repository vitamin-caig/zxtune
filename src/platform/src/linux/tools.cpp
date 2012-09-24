/*
Abstract:
  Linux implementation of tools

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <tools.h>
//library includes
#include <platform/tools.h>
//boost includes
#include <boost/format.hpp>
//platform includes
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>

namespace
{
  std::string GetSharedLibraryName()
  {
    Dl_info info;
    return 0 != ::dladdr(reinterpret_cast<void*>(&GetSharedLibraryName), &info)
      ? std::string(info.dli_fname)
      : std::string();
  }
  
  std::string GetExecutableName()
  {
    const std::string selfPath = (boost::format("/proc/%1%/exe") % ::getpid()).str();
    struct stat sb;
    if (-1 == ::lstat(selfPath.c_str(), &sb))
    {
      return std::string();
    }
    
    std::vector<char> filename(sb.st_size + 1);
    const int len = ::readlink(selfPath.c_str(), &filename[0], filename.size() - 1);
    if (len > 0)
    {
      filename[len] = 0;
      return std::string(&filename[0]);
    }
    else
    {
      return std::string();
    }
  }
}

namespace Platform
{
  std::string GetCurrentImageFilename()
  {
    static const std::string soName = GetSharedLibraryName();
    if (!soName.empty())
    {
      return soName;
    }
    static const std::string binName = GetExecutableName();
    return binName;
  }
}
