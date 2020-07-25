/**
*
* @file
*
* @brief  Windows implementation of tools
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <platform/tools.h>
//std includes
#include <array>
//platform includes
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
}

namespace Platform
{
  std::string GetCurrentImageFilename()
  {
    const HMODULE mod = GetCurrentModule();
    std::array<char, MAX_PATH + 1> buff;
    const uint_t size = ::GetModuleFileName(mod, buff.data(), static_cast<DWORD>(MAX_PATH));
    buff[size] = 0;
    return std::string(buff.data());
  }
}
