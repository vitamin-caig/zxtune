/**
 *
 * @file
 *
 * @brief  Main program function
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "debug/log.h"
#include "platform/application.h"
#include "strings/array.h"

#include "error.h"
#include "string_view.h"

#include <exception>
#include <iostream>
#include <locale>
#include <memory>

std::ostream& StdOut = std::cout;

namespace
{
  const Debug::Stream Dbg("Platform::Application");
}  // namespace

// TODO: extract to different sources
#ifdef _WIN32
#  include "strings/encoding.h"

#  include "pointers.h"

#  include <windows.h>

Strings::Array ParseArgv(int, const char**)
{
  int argc = 0;
  const std::shared_ptr<const LPWSTR> cmdline(::CommandLineToArgvW(::GetCommandLineW(), &argc), &::LocalFree);
  if (!cmdline)
  {
    throw std::runtime_error("Failed to get argv");
  }
  const auto argv = cmdline.get();
  Strings::Array res(argc);
  for (int idx = 0; idx < argc; ++idx)
  {
    static_assert(sizeof(**argv) == sizeof(uint16_t), "Incompatible wchar_t");
    res[idx] = Strings::Utf16ToUtf8(safe_ptr_cast<const uint16_t*>(argv[idx]));
  }
  return res;
}
#else
Strings::Array ParseArgv(int argc, const char* argv[])
{
  return {argv, argv + argc};
}
#endif

void SetupLocale()
{
  try
  {
    // Required for Darwin but fails on clang-mingw toolchain
    std::locale::global(std::locale("C"));
  }
  catch (const std::exception& e)
  {
    Dbg("Failed to setup locale: {}", e.what());
  }
  catch (...)
  {
    Dbg("Failed to setup locale");
  }
}

int main(int argc, const char* argv[])
{
  try
  {
    SetupLocale();
    const auto app = Platform::Application::Create();
    return app->Run(ParseArgv(argc, argv));
  }
  catch (const Error& e)
  {
    StdOut << e.ToString() << std::endl;
    return 1;
  }
  catch (const std::exception& e)
  {
    StdOut << e.what() << std::endl;
    return 1;
  }
  catch (...)
  {
    StdOut << "Unhandled exception" << std::endl;
    return 1;
  }
}
