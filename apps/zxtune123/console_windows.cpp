#include "console.h"

#include <types.h>

#ifndef _WIN32
#error Invalid platform specified
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <conio.h>
#include <windows.h>

#include <boost/array.hpp>

#include <cctype>

void GetConsoleSize(std::pair<int, int>& sizes)
{
  CONSOLE_SCREEN_BUFFER_INFO info;
  const HANDLE hdl(::GetStdHandle(STD_OUTPUT_HANDLE));
  ::GetConsoleScreenBufferInfo(hdl, &info);
  sizes = std::pair<int, int>(info.srWindow.Right - info.srWindow.Left - 1, info.srWindow.Bottom - info.srWindow.Top - 1);
}

void MoveCursorUp(int lines)
{
  CONSOLE_SCREEN_BUFFER_INFO info;
  const HANDLE hdl(::GetStdHandle(STD_OUTPUT_HANDLE));
  ::GetConsoleScreenBufferInfo(hdl, &info);
  info.dwCursorPosition.Y -= lines;
  info.dwCursorPosition.X = 0;
  ::SetConsoleCursorPosition(hdl, info.dwCursorPosition);
}

unsigned GetPressedKey()
{
  if (::_kbhit())
  {
    switch (const int code = ::_getch())
    {
    case VK_ESCAPE:
      return KEY_CANCEL;
    case VK_RETURN:
      return KEY_ENTER;
    case 0x00:
    case 0xe0:
      {
        switch (const int code2 = ::_getch())
        {
        case 72:
          return KEY_UP;
        case 75:
          return KEY_LEFT;
        case 77:
          return KEY_RIGHT;
        case 80:
          return KEY_DOWN;
        default:
          return KEY_NONE;
        }
      }
    default:
      return std::toupper(code);
    };
  }
  return KEY_NONE;
}
