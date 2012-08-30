/*
Abstract:
  SharedLibrary implementation for Windows

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "shared_library_common.h"
//common includes
#include <contract.h>
#include <error_tools.h>
//library includes
#include <l10n/api.h>
//boost includes
#include <boost/make_shared.hpp>
//platform includes
#include <windows.h>
//text includes
#include <tools/text/tools.h>

#define FILE_TAG 326CACD8

namespace
{
  const unsigned THIS_MODULE = Error::ModuleCode<'W', 'S', 'O'>::Value;

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("tools");

  class WindowsSharedLibrary : public SharedLibrary
  {
  public:
    explicit WindowsSharedLibrary(HMODULE handle)
      : Handle(handle)
    {
      Require(Handle != 0);
    }

    virtual ~WindowsSharedLibrary()
    {
      if (Handle)
      {
        ::FreeLibrary(Handle);
      }
    }

    virtual void* GetSymbol(const std::string& name) const
    {
      if (void* res = reinterpret_cast<void*>(::GetProcAddress(Handle, name.c_str())))
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE, THIS_MODULE, 
        translate("Failed to find symbol '%1%' in dynamic library."), FromStdString(name));
    }
  private:
    const HMODULE Handle;
  };

  //TODO: String GetWindowsError()
  uint_t GetWindowsError()
  {
    return ::GetLastError();
  }

  const std::string SUFFIX(".dll");
  
  std::string BuildLibraryFilename(const std::string& name)
  {
    return name + SUFFIX;
  }
}

Error LoadSharedLibrary(const std::string& fileName, SharedLibrary::Ptr& res)
{
  if (HMODULE handle = ::LoadLibrary(fileName.c_str()))
  {
    res = boost::make_shared<WindowsSharedLibrary>(handle);
    return Error();
  }
  return MakeFormattedError(THIS_LINE, THIS_MODULE,
    translate("Failed to load dynamic library '%1%' (error code is %2%)."), FromStdString(fileName), GetWindowsError());
}


std::string GetSharedLibraryFilename(const std::string& name)
{
  return name.find(SUFFIX) == name.npos
    ? BuildLibraryFilename(name)
    : name;
}

std::vector<std::string> GetSharedLibraryFilenames(const SharedLibrary::Name& name)
{
  std::vector<std::string> res;
  res.push_back(GetSharedLibraryFilename(name.Base()));
  const std::vector<std::string>& alternatives = name.WindowsAlternatives();
  std::transform(alternatives.begin(), alternatives.end(), std::back_inserter(res), std::ptr_fun(&GetSharedLibraryFilename));
  return res;
}
