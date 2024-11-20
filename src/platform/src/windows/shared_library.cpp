/**
 *
 * @file
 *
 * @brief  SharedLibrary implementation for Windows
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "platform/src/shared_library_common.h"

#include "l10n/api.h"

#include "contract.h"
#include "error_tools.h"
#include "make_ptr.h"
#include "string_view.h"

#include <windows.h>

#include <algorithm>

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

  const auto SUFFIX = ".dll"sv;

  String BuildLibraryFilename(StringView name)
  {
    // TODO: Concat(StringView...)
    return String(name).append(SUFFIX);
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

  String GetSharedLibraryFilename(StringView name)
  {
    return name.find(SUFFIX) == name.npos ? BuildLibraryFilename(name) : String{name};
  }

  std::vector<String> GetSharedLibraryFilenames(const SharedLibrary::Name& name)
  {
    std::vector<String> res;
    res.emplace_back(GetSharedLibraryFilename(name.Base()));
    for (const auto& alt : name.WindowsAlternatives())
    {
      res.emplace_back(GetSharedLibraryFilename(alt));
    }
    return res;
  }
}  // namespace Platform::Details
