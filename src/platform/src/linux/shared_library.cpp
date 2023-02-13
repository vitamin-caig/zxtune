/**
 *
 * @file
 *
 * @brief  SharedLibrary implementation for Linux
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "platform/src/shared_library_common.h"
// common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <l10n/api.h>
// platform includes
#include <dlfcn.h>

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("platform");
}

namespace Platform::Details
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

    void* GetSymbol(const String& name) const override
    {
      if (void* res = ::dlsym(Handle, name.c_str()))
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE, translate("Failed to find symbol '{}' in shared object."), name);
    }

  private:
    void* const Handle;
  };

  const String SUFFIX(".so");

  String BuildLibraryFilename(const String& name)
  {
    return "lib" + name + SUFFIX;
  }

  Error LoadSharedLibrary(const String& fileName, SharedLibrary::Ptr& res)
  {
    if (void* handle = ::dlopen(fileName.c_str(), RTLD_LAZY))
    {
      res = MakePtr<LinuxSharedLibrary>(handle);
      return Error();
    }
    return MakeFormattedError(THIS_LINE, translate("Failed to load shared object '{0}' ({1})."), fileName, ::dlerror());
  }

  String GetSharedLibraryFilename(const String& name)
  {
    return name.find(SUFFIX) == name.npos ? BuildLibraryFilename(name) : name;
  }

  std::vector<String> GetSharedLibraryFilenames(const SharedLibrary::Name& name)
  {
    std::vector<String> res;
    res.push_back(GetSharedLibraryFilename(name.Base()));
    const auto& alternatives = name.PosixAlternatives();
    std::copy(alternatives.begin(), alternatives.end(), std::back_inserter(res));
    return res;
  }
}  // namespace Platform::Details
