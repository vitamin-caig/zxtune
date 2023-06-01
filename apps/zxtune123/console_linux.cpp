/**
 *
 * @file
 *
 * @brief Console implementation for linux
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "console.h"
// common includes
#include <error.h>
// library includes
#include <platform/application.h>
// platform-dependent includes
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
// std includes
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>

namespace
{
  void ThrowIfError(int res, Error::LocationRef loc)
  {
    if (res)
    {
      throw Error(loc, ::strerror(errno));
    }
  }

  unsigned ReadKey()
  {
    uint8_t codes[3] = {};
    switch (::read(STDIN_FILENO, codes, 3))
    {
    case 3:
      return 65536 * codes[0] + 256 * codes[1] + codes[2];
    case 2:
      return 256 * codes[0] + codes[1];
    case 1:
      return codes[0];
    default:
      return 0;
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
        newProps.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | INLCR | IGNCR | IXON);
        newProps.c_cc[VMIN] = 0;
        newProps.c_cc[VTIME] = 0;
        ThrowIfError(::tcsetattr(STDIN_FILENO, TCSANOW, &newProps), THIS_LINE);
      }
    }

    ~LinuxConsole() override
    {
      // not throw
      if (IsConsoleIn)
      {
        ::tcsetattr(STDIN_FILENO, TCSANOW, &OldProps);
      }
    }

    SizeType GetSize() const override
    {
      if (!IsConsoleOut)
      {
        return {-1, -1};
      }
#if defined TIOCGSIZE
      struct ttysize ts;
      ThrowIfError(ioctl(STDOUT_FILENO, TIOCGSIZE, &ts), THIS_LINE);
      return {ts.ts_cols, ts.ts_lines};
#elif defined TIOCGWINSZ
      struct winsize ws;
      ThrowIfError(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws), THIS_LINE);
      return {ws.ws_col, ws.ws_row};
#else
      return {-1, -1};
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

      switch (ReadKey())
      {
      case 'q':
        return 'Q';
      case 0x20:
        return ' ';
      case 0x1b:
        return INPUT_KEY_CANCEL;
      case 0x1b5b41:
        return INPUT_KEY_UP;
      case 0x1b5b42:
        return INPUT_KEY_DOWN;
      case 0x1b5b43:
        return INPUT_KEY_RIGHT;
      case 0x1b5b44:
        return INPUT_KEY_LEFT;
      case 0x0a:
        return INPUT_KEY_ENTER;
      default:
        return INPUT_KEY_NONE;
      }
    }

    void WaitForKeyRelease() const override
    {
      if (IsConsoleIn)
      {
        while (0 != ReadKey())
        {}
      }
    }

    void Write(StringView str) const override
    {
      StdOut << str;
    }

  private:
    struct termios OldProps;
    const bool IsConsoleIn;
    const bool IsConsoleOut;
  };
}  // namespace

Console& Console::Self()
{
  static LinuxConsole self;
  return self;
}
