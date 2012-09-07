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
//platform includes
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>

namespace
{
  String GetSharedLibraryName()
  {
    Dl_info info;
    return 0 != ::dladdr(reinterpret_cast<void*>(&GetSharedLibraryName), &info)
      ? FromStdString(info.dli_fname)
      : String();
  }
  
  String GetExecutableName()
  {
    static const std::string SELF_PATH("/proc/self/exe");
    struct stat sb;
    if (-1 == ::lstat(SELF_PATH.c_str(), &sb))
    {
      return String();
    }
    
    std::vector<char> filename(sb.st_size + 1);
    const int len = ::readlink("/proc/self/exe", &filename[0], filename.size() - 1);
    if (len > 0)
    {
      filename[len] = 0;
      return FromStdString(&filename[0]);
    }
    else
    {
      return String();
    }
  }
}

namespace Platform
{
  String GetCurrentImageFilename()
  {
    static const String soName = GetSharedLibraryName();
    if (!soName.empty())
    {
      return soName;
    }
    static const String binName = GetExecutableName();
    return binName;
  }
}
