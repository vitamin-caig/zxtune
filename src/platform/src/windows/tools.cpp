/*
Abstract:
  Windows implementation of tools

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
    char buff[MAX_PATH + 1];
    const uint_t size = ::GetModuleFileName(mod, &buff[0], static_cast<DWORD>(ArraySize(buff) - 1));
    buff[size] = 0;
    return std::string(buff);
  }
}
