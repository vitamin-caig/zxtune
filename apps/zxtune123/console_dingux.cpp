/**
* 
* @file
*
* @brief Console implementation for dingux
*
* @author vitamin.caig@gmail.com
*
**/

#ifndef __linux__
#error Invalid platform specified
#endif

//local includes
#include "console.h"
//common includes
#include <error.h>
//library includes
#include <platform/application.h>
//platform-dependent includes
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <termio.h>
#include <linux/input.h>
//std includes
#include <iostream>
#include <thread>

#define FILE_TAG B2767AB0

namespace
{
  void ThrowIfError(int res, Error::LocationRef loc)
  {
    if (res)
    {
      throw Error(loc, ::strerror(errno));
    }
  }
  
  class DinguxConsole : public Console
  {
  public:
    DinguxConsole()
      : EventHandle(::open("/dev/event0", O_RDONLY | O_NONBLOCK))
      , IsConsoleIn(::isatty(STDIN_FILENO))
      , IsConsoleOut(::isatty(STDOUT_FILENO))
    {
      ThrowIfError(EventHandle < 0, THIS_LINE);
      //disable key echo
      if (IsConsoleIn)
      {
        ThrowIfError(::tcgetattr(STDIN_FILENO, &OldProps), THIS_LINE);
        struct termios newProps = OldProps;
        newProps.c_lflag &= ~(ICANON | ECHO);
        newProps.c_cc[VMIN] = 0;
        newProps.c_cc[VTIME] = 1;
        ThrowIfError(::tcsetattr(STDIN_FILENO, TCSANOW, &newProps), THIS_LINE);
      }
      if (IsConsoleOut)
      {
#if defined TIOCGSIZE
        struct ttysize ts;
        ThrowIfError(ioctl(STDOUT_FILENO, TIOCGSIZE, &ts), THIS_LINE);
        ConsoleSize = SizeType(ts.ts_cols, ts.rs_rows);
#elif defined TIOCGWINSZ
        struct winsize ws;
        ThrowIfError(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws), THIS_LINE);
        ConsoleSize = SizeType(ws.ws_col, ws.ws_row);
#else
#error Unknown console mode for dingux
#endif
      }
      else
      {
        ConsoleSize = SizeType(-1, -1);
      }
    }

    virtual ~DinguxConsole()
    {
      if (IsConsoleIn)
      {
        ::tcsetattr(STDIN_FILENO, TCSANOW, &OldProps);
      }
      if (-1 != EventHandle)
      {
        ::close(EventHandle);//do not throw
      }
    }

    virtual SizeType GetSize() const
    {
      return ConsoleSize;
    }

    virtual void MoveCursorUp(uint_t lines)
    {
      if (IsConsoleOut)
      {
        StdOut << "\x1b[" << lines << 'A' << std::flush;
      }
    }

    virtual uint_t GetPressedKey() const
    {
      struct input_event event;
      if (::read(EventHandle, &event, sizeof(event)) != sizeof(event) ||
          event.type != EV_KEY ||
          event.value != 1) //pressed
      {
        return INPUT_KEY_NONE;
      }

      switch (event.code)
      {
      case KEY_ESC:
        return INPUT_KEY_CANCEL;
      case KEY_LEFT:
        return INPUT_KEY_LEFT;
      case KEY_RIGHT:
        return INPUT_KEY_RIGHT;
      case KEY_UP:
      case KEY_BACKSPACE:
        return INPUT_KEY_UP;
      case KEY_DOWN:
      case KEY_TAB:
        return INPUT_KEY_DOWN;
      case KEY_ENTER:
      case KEY_LEFTALT:
        return INPUT_KEY_ENTER;
      default:
        return ' ';
      }
    }

    virtual void WaitForKeyRelease() const
    {
      struct input_event event;
      while (::read(EventHandle, &event, sizeof(event)) != sizeof(event) ||
          event.type != EV_KEY)
      {
        if (event.value == 0)
        {
          break;
        }
        //TODO: poll for event
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }
  private:
    const int EventHandle;
    const bool IsConsoleIn;
    const bool IsConsoleOut;
    struct termios OldProps;
    SizeType ConsoleSize;
  };
}

Console& Console::Self()
{
  static DinguxConsole self;
  return self;
}
