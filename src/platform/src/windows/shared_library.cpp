/**
 *
 * @file
 *
 * @brief  SharedLibrary implementation for Windows
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
// std includes
#include <algorithm>
// platform includes
#include <windows.h>

#define FILE_TAG 326CACD8

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("platform");
}

namespace Platform::Details
{
  class WindowsSharedLibrary : public SharedLibrary
  {
  public:
    explicit WindowsSharedLibrary(HMODULE handle)
      : Handle(handle)
    {
      Require(Handle != 0);
    }

    ~WindowsSharedLibrary() override
    {
      if (Handle)
      {
        ::FreeLibrary(Handle);
      }
    }

    void* GetSymbol(const String& name) const override
    {
      if (void* res = reinterpret_cast<void*>(::GetProcAddress(Handle, name.c_str())))
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE, translate("Failed to find symbol '{}' in dynamic library."), name);
    }

  private:
    const HMODULE Handle;
  };

  // TODO: String GetWindowsError()
  uint_t GetWindowsError()
  {
    return ::GetLastError();
  }

  const String SUFFIX(".dll");

  String BuildLibraryFilename(const String& name)
  {
    return name + SUFFIX;
  }

  Error LoadSharedLibrary(const String& fileName, SharedLibrary::Ptr& res)
  {
    if (HMODULE handle = ::LoadLibrary(fileName.c_str()))
    {
      res = MakePtr<WindowsSharedLibrary>(handle);
      return Error();
    }
    return MakeFormattedError(THIS_LINE, translate("Failed to load dynamic library '{0}' (error code is {1})."),
                              fileName, GetWindowsError());
  }

  String GetSharedLibraryFilename(const String& name)
  {
    return name.find(SUFFIX) == name.npos ? BuildLibraryFilename(name) : name;
  }

  std::vector<String> GetSharedLibraryFilenames(const SharedLibrary::Name& name)
  {
    std::vector<String> res;
    res.push_back(GetSharedLibraryFilename(name.Base()));
    const auto& alternatives = name.WindowsAlternatives();
    std::transform(alternatives.begin(), alternatives.end(), std::back_inserter(res), &GetSharedLibraryFilename);
    return res;
  }
}  // namespace Platform::Details
