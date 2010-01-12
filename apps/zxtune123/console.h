#ifndef ZXTUNE123_CONSOLE_H_DEFINED
#define ZXTUNE123_CONSOLE_H_DEFINED

#include <utility>

class Console
{
public:
  virtual ~Console() {}

  virtual std::pair<int, int> GetSize() const = 0;
  virtual void MoveCursorUp(int lines) = 0;

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
  virtual unsigned GetPressedKey() const = 0;
  
  virtual void WaitForKeyRelease() const = 0;
  
  static Console& Self();
};

#endif //#ifndef ZXTUNE123_CONSOLE_H_DEFINED
