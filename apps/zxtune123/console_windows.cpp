#include "console.h"

#ifndef _WIN32
#error Invalid platform specified
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <conio.h>
#include <windows.h>

void GetConsoleSize(std::pair<int, int>& sizes)
{
  CONSOLE_SCREEN_BUFFER_INFO info;
  HANDLE hdl(GetStdHandle(STD_OUTPUT_HANDLE));
  GetConsoleScreenBufferInfo(hdl, &info);
  sizes = std::pair<int, int>(info.srWindow.Right - info.srWindow.Left - 1, info.srWindow.Bottom - info.srWindow.Top - 1);
}

void MoveCursorUp(int lines)
{
  CONSOLE_SCREEN_BUFFER_INFO info;
  HANDLE hdl(GetStdHandle(STD_OUTPUT_HANDLE));
  GetConsoleScreenBufferInfo(hdl, &info);
  info.dwCursorPosition.Y -= lines;
  info.dwCursorPosition.X = 0;
  SetConsoleCursorPosition(hdl, info.dwCursorPosition);
}
