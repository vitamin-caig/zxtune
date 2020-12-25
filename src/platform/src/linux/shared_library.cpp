/**
*
* @file
*
* @brief  SharedLibrary implementation for Linux
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "platform/src/shared_library_common.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <l10n/api.h>
//platform includes
#include <dlfcn.h>

#define FILE_TAG 4C0042B0

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("platform");
}

namespace Platform
{
namespace Details
{
  class LinuxSharedLibrary : public SharedLibrary
  {
  public:
    explicit LinuxSharedLibrary(void* handle)
      : Handle(handle)
    {
      Require(Handle != nullptr);
    }

    ~LinuxSharedLibrary() override
    {
      if (Handle)
      {
        ::dlclose(Handle);
      }
    }

    void* GetSymbol(const std::string& name) const override
    {
      if (void* res = ::dlsym(Handle, name.c_str()))
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE,
        translate("Failed to find symbol '%1%' in shared object."), name);
    }
  private:
    void* const Handle;
  };
  
  const std::string SUFFIX(".so");
  
  std::string BuildLibraryFilename(const std::string& name)
  {
    return "lib" + name + SUFFIX;
  }

  Error LoadSharedLibrary(const std::string& fileName, SharedLibrary::Ptr& res)
  {
    if (void* handle = ::dlopen(fileName.c_str(), RTLD_LAZY))
    {
      res = MakePtr<LinuxSharedLibrary>(handle);
      return Error();
    }
    return MakeFormattedError(THIS_LINE,
      translate("Failed to load shared object '%1%' (%2%)."), fileName, ::dlerror());
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
    std::copy(alternatives.begin(), alternatives.end(), std::back_inserter(res));
    return res;
  }
}
}
