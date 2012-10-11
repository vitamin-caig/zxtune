/*
Abstract:
  SharedLibrary implementation for Linux

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "platform/src/shared_library_common.h"
//common includes
#include <contract.h>
#include <error_tools.h>
//library includes
#include <l10n/api.h>
//boost includes
#include <boost/make_shared.hpp>
//platform includes
#include <dlfcn.h>

#define FILE_TAG 4C0042B0

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("platform");

  class LinuxSharedLibrary : public Platform::SharedLibrary
  {
  public:
    explicit LinuxSharedLibrary(void* handle)
      : Handle(handle)
    {
      Require(Handle != 0);
    }

    virtual ~LinuxSharedLibrary()
    {
      if (Handle)
      {
        ::dlclose(Handle);
      }
    }

    virtual void* GetSymbol(const std::string& name) const
    {
      if (void* res = ::dlsym(Handle, name.c_str()))
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE,
        translate("Failed to find symbol '%1%' in shared object."), FromStdString(name));
    }
  private:
    void* const Handle;
  };
  
  const std::string SUFFIX(".so");
  
  std::string BuildLibraryFilename(const std::string& name)
  {
    return "lib" + name + SUFFIX;
  }
}

namespace Platform
{
  namespace Detail
  {
    Error LoadSharedLibrary(const std::string& fileName, SharedLibrary::Ptr& res)
    {
      if (void* handle = ::dlopen(fileName.c_str(), RTLD_LAZY))
      {
        res = boost::make_shared<LinuxSharedLibrary>(handle);
        return Error();
      }
      return MakeFormattedError(THIS_LINE,
        translate("Failed to load shared object '%1%' (%2%)."), FromStdString(fileName), FromStdString(::dlerror()));
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
      const std::vector<std::string>& alternatives = name.PosixAlternatives();
      std::transform(alternatives.begin(), alternatives.end(), std::back_inserter(res), std::ptr_fun(&GetSharedLibraryFilename));
      return res;
    }
  }
}
