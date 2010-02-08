/*
Abstract:
  Console operation implementation for windows

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
#include "console.h"

#ifndef _WIN32
#error Invalid platform specified
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <conio.h>
#include <windows.h>

#include <boost/array.hpp>

#include <cctype>

namespace
{
  class WindowsConsole : public Console
  {
  public:
    WindowsConsole()
      : Handle(::GetStdHandle(STD_OUTPUT_HANDLE))
    {
    }

    virtual SizeType GetSize() const
    {
      CONSOLE_SCREEN_BUFFER_INFO info;
      ::GetConsoleScreenBufferInfo(Handle, &info);
      return SizeType(info.srWindow.Right - info.srWindow.Left - 1,
        info.srWindow.Bottom - info.srWindow.Top - 1);
    }

    virtual void MoveCursorUp(uint_t lines)
    {
      CONSOLE_SCREEN_BUFFER_INFO info;
      ::GetConsoleScreenBufferInfo(Handle, &info);
      info.dwCursorPosition.Y -= static_cast< ::SHORT>(lines);
      info.dwCursorPosition.X = 0;
      ::SetConsoleCursorPosition(Handle, info.dwCursorPosition);
    }

    virtual uint_t GetPressedKey() const
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
            switch (::_getch())
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

    virtual void WaitForKeyRelease() const
    {
      while (::_kbhit())
      {
      }
    }
  private:
    const HANDLE Handle;
  };
}

Console& Console::Self()
{
  static WindowsConsole self;
  return self;
}
