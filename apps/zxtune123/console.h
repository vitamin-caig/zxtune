#ifndef ZXTUNE123_CONSOLE_H_DEFINED
#define ZXTUNE123_CONSOLE_H_DEFINED

#include <utility>

void GetConsoleSize(std::pair<int, int>& sizes);
void MoveCursorUp(int lines);

enum
{
  KEY_NONE = 0,
  KEY_LEFT = 8,
  KEY_RIGHT = 9,
  KEY_UP = 10,
  KEY_DOWN = 11,
  KEY_CANCEL = 12,
  KEY_ENTER = 13,
};
unsigned GetPressedKey();

#endif //#ifndef ZXTUNE123_CONSOLE_H_DEFINED
