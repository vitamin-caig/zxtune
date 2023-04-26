/**
 *
 * @file
 *
 * @brief Console interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// std includes
#include <utility>

class Console
{
public:
  virtual ~Console() = default;

  typedef std::pair<int_t, int_t> SizeType;
  virtual SizeType GetSize() const = 0;
  virtual void MoveCursorUp(uint_t lines) = 0;

  enum
  {
    INPUT_KEY_NONE = 0,
    INPUT_KEY_LEFT = 8,
    INPUT_KEY_RIGHT = 9,
    INPUT_KEY_UP = 10,
    INPUT_KEY_DOWN = 11,
    INPUT_KEY_CANCEL = 12,
    INPUT_KEY_ENTER = 13,
  };
  virtual uint_t GetPressedKey() const = 0;

  virtual void WaitForKeyRelease() const = 0;

  virtual void Write(StringView str) const = 0;

  static Console& Self();
};
