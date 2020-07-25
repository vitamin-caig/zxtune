/**
* 
* @file
*
* @brief Console implementation for linux
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "console.h"
//common includes
#include <error.h>
//library includes
#include <platform/application.h>
//platform-dependent includes
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
//std includes
#include <iostream>

#define FILE_TAG D037A662

namespace
{
  void ThrowIfError(int res, Error::LocationRef loc)
  {
    if (res)
    {
      throw Error(loc, ::strerror(errno));
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
        newProps.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | INLCR | IGNCR | ICRNL | IXON);
        newProps.c_cc[VMIN] = 0;
        newProps.c_cc[VTIME] = 0;
        ThrowIfError(::tcsetattr(STDIN_FILENO, TCSANOW, &newProps), THIS_LINE);
      }
    }
    
    ~LinuxConsole() override
    {
      //not throw
      if (IsConsoleIn)
      {
        ::tcsetattr(STDIN_FILENO, TCSANOW, &OldProps);
      }
    }
    
    SizeType GetSize() const override
    {
      if (!IsConsoleOut)
      {
        return SizeType(-1, -1);
      }
#if defined TIOCGSIZE
      struct ttysize ts;
      ThrowIfError(ioctl(STDOUT_FILENO, TIOCGSIZE, &ts), THIS_LINE);
      return SizeType(ts.ts_cols, ts.ts_lines);
#elif defined TIOCGWINSZ
      struct winsize ws;
      ThrowIfError(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws), THIS_LINE);
      return SizeType(ws.ws_col, ws.ws_row);
#else
      return SizeType(-1, -1);
#endif
    }
    
    void MoveCursorUp(uint_t lines) override
    {
      if (IsConsoleOut)
      {
        StdOut << "\x1b[" << lines << 'A' << std::flush;
      }
    }

    uint_t GetPressedKey() const override
    {
      if (!IsConsoleIn)
      {
        return INPUT_KEY_NONE;
      }
       
      switch (const int code = ::getchar())
      {
      case -1:
        return INPUT_KEY_NONE;
      case 27:
        {
          if (::getchar() != '[')
          {
            return INPUT_KEY_NONE;
          }
          switch (::getchar())
          {
          case 65:
            return INPUT_KEY_UP;
          case 66:
            return INPUT_KEY_DOWN;
          case 67:
            return INPUT_KEY_RIGHT;
          case 68:
            return INPUT_KEY_LEFT;
          default:
            return INPUT_KEY_NONE;
          }
        }
      case 10:
        return INPUT_KEY_ENTER;
      default:
        return std::toupper(code);
      };
    }
    
    void WaitForKeyRelease() const override
    {
      if (IsConsoleIn)
      {
        for (int key = ::getchar(); -1 != key; key = ::getchar())
        {
        }
      }
    }

    void Write(const String& str) const override
    {
      StdOut << str;
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
