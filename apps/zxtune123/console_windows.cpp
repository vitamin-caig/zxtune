/**
 *
 * @file
 *
 * @brief Console implementation for windows
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#ifndef _WIN32
#  error Invalid platform specified
#endif

// local includes
#include "console.h"
// platform-dependent includes
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <conio.h>
#include <windows.h>
// std includes
#include <cctype>
#include <vector>

namespace
{
  class WindowsConsole : public Console
  {
  public:
    WindowsConsole()
      : Handle(::GetStdHandle(STD_OUTPUT_HANDLE))
    {}

    SizeType GetSize() const override
    {
      CONSOLE_SCREEN_BUFFER_INFO info;
      ::GetConsoleScreenBufferInfo(Handle, &info);
      return SizeType(info.srWindow.Right - info.srWindow.Left - 1, info.srWindow.Bottom - info.srWindow.Top - 1);
    }

    void MoveCursorUp(uint_t lines) override
    {
      CONSOLE_SCREEN_BUFFER_INFO info;
      ::GetConsoleScreenBufferInfo(Handle, &info);
      info.dwCursorPosition.Y -= static_cast< ::SHORT>(lines);
      info.dwCursorPosition.X = 0;
      ::SetConsoleCursorPosition(Handle, info.dwCursorPosition);
    }

    uint_t GetPressedKey() const override
    {
      if (::_kbhit())
      {
        switch (const int code = ::_getch())
        {
        case VK_ESCAPE:
          return INPUT_KEY_CANCEL;
        case VK_RETURN:
          return INPUT_KEY_ENTER;
        case 0x00:
        case 0xe0:
        {
          switch (::_getch())
          {
          case 72:
            return INPUT_KEY_UP;
          case 75:
            return INPUT_KEY_LEFT;
          case 77:
            return INPUT_KEY_RIGHT;
          case 80:
            return INPUT_KEY_DOWN;
          default:
            return INPUT_KEY_NONE;
          }
        }
        default:
          return std::toupper(code);
        };
      }
      return INPUT_KEY_NONE;
    }

    void WaitForKeyRelease() const override
    {
      while (::_kbhit())
      {}
    }

    void Write(StringView str) const override
    {
      std::vector<wchar_t> wide(str.size());
      const auto wideSize = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), wide.data(), wide.size());
      std::vector<char> narrow(str.size());
      const auto narrowSize =
          ::WideCharToMultiByte(CP_OEMCP, 0, wide.data(), wideSize, narrow.data(), narrow.size(), 0, 0);
      DWORD realSize = 0;
      ::WriteFile(Handle, narrow.data(), narrowSize, &realSize, NULL);
    }

  private:
    const HANDLE Handle;
  };
}  // namespace

Console& Console::Self()
{
  static WindowsConsole self;
  return self;
}
