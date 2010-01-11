#include "console.h"
#include "error_codes.h"

#ifndef __linux__
#error Invalid platform specified
#endif

#include <errno.h>
#include <termio.h>

#include <iostream>

void ThrowIfError(int res, Error::LocationRef loc)
{
  if (res)
  {
    throw Error(loc, UNKNOWN_ERROR, ::strerror(errno));
  }
}

void GetConsoleSize(std::pair<int, int>& sizes)
{
#if defined TIOCGSIZE
  struct ttysize ts;
  ThrowIfError(ioctl(STDOUT_FILENO, TIOCGSIZE, &ts), THIS_LINE);
  sizes = std::make_pair(ts.ts_cols, ts.rs_rows);
#elif defined TIOCGWINSZ
  struct winsize ws;
  ThrowIfError(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws), THIS_LINE);
  sizes = std::make_pair(ws.ws_col, ws.ws_row);
#else
#error Unknown console mode for linux
#endif
}

void MoveCursorUp(int lines)
{
  std::cout << "\x1b[" << lines << 'A' << std::flush;
}
