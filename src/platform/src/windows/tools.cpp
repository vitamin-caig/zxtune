/**
 *
 * @file
 *
 * @brief  Windows implementation of tools
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <platform/tools.h>
// platform includes
#include <windows.h>

namespace
{
  HMODULE GetCurrentModule()
  {
    HMODULE res = NULL;
#if _WIN32_WINNT > 0x500
    ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCTSTR>(&GetCurrentModule), &res);
#endif
    return res;
  }
}  // namespace

namespace Platform
{
  String GetCurrentImageFilename()
  {
    const auto mod = GetCurrentModule();
    String result(MAX_PATH + 1, ' ');
    const auto size = ::GetModuleFileName(mod, result.data(), static_cast<DWORD>(MAX_PATH));
    result.resize(size);
    return result;
  }
}  // namespace Platform
