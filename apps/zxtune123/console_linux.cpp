/*
Abstract:
  Console operation implementation for linux

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/

#include "app.h"
#include "console.h"
#include "error_codes.h"

#ifndef __linux__
#error Invalid platform specified
#endif

#include <errno.h>
#include <stdio.h>
#include <termio.h>

#include <iostream>

#define FILE_TAG D037A662

namespace
{
  void ThrowIfError(int res, Error::LocationRef loc)
  {
    if (res)
    {
      throw Error(loc, UNKNOWN_ERROR, ::strerror(errno));
    }
  }
  
  class LinuxConsole : public Console
  {
  public:
    LinuxConsole()
      : IsConsoleIn(::isatty(STDIN_FILENO))
      , IsConsoleOut(::isatty(STDOUT_FILENO))
    {
      if (IsConsoleIn)
      {
        ThrowIfError(::tcgetattr(STDIN_FILENO, &OldProps), THIS_LINE);
        struct termios newProps = OldProps;
        newProps.c_lflag &= ~(ICANON | ECHO);
        newProps.c_cc[VMIN] = 0;
        newProps.c_cc[VTIME] = 1;
        ThrowIfError(::tcsetattr(STDIN_FILENO, TCSANOW, &newProps), THIS_LINE);
      }
    }
    
    virtual ~LinuxConsole()
    {
      //not throw
      if (IsConsoleIn)
      {
        ::tcsetattr(STDIN_FILENO, TCSANOW, &OldProps);
      }
    }
    
    virtual std::pair<int, int> GetSize() const
    {
      if (!IsConsoleOut)
      {
        return std::make_pair(-1, -1);
      }
#if defined TIOCGSIZE
      struct ttysize ts;
      ThrowIfError(ioctl(STDOUT_FILENO, TIOCGSIZE, &ts), THIS_LINE);
      return std::make_pair(ts.ts_cols, ts.rs_rows);
#elif defined TIOCGWINSZ
      struct winsize ws;
      ThrowIfError(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws), THIS_LINE);
      return std::make_pair(ws.ws_col, ws.ws_row);
#else
#error Unknown console mode for linux
#endif
    }
    
    virtual void MoveCursorUp(int lines)
    {
      if (IsConsoleOut)
      {
        StdOut << "\x1b[" << lines << 'A' << std::flush;
      }
    }

    virtual unsigned GetPressedKey() const
    {
      if (!IsConsoleIn)
      {
        return KEY_NONE;
      }
        
      switch (const int code = ::getchar())
      {
      case -1:
        return KEY_NONE;
      case 27:
        {
          if (::getchar() != '[')
          {
            return KEY_NONE;
          }
          switch (::getchar())
          {
          case 65:
            return KEY_UP;
          case 66:
            return KEY_DOWN;
          case 67:
            return KEY_RIGHT;
          case 68:
            return KEY_LEFT;
          default:
            return KEY_NONE;
          }
        }
      case 10:
        return KEY_ENTER;
      default:
        return std::toupper(code);
      };
    }
    
    virtual void WaitForKeyRelease() const
    {
      if (IsConsoleIn)
      {
        for (int key = ::getchar(); -1 != key; key = ::getchar())
        {
        }
      }
    }
  private:
    struct termios OldProps;
    const bool IsConsoleIn;
    const bool IsConsoleOut;
  };
}

Console& Console::Self()
{
  static LinuxConsole self;
  return self;
}
