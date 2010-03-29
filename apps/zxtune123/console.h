/*
Abstract:
  Console operation interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
#ifndef ZXTUNE123_CONSOLE_H_DEFINED
#define ZXTUNE123_CONSOLE_H_DEFINED

#include <types.h>

#include <utility>

class Console
{
public:
  virtual ~Console() {}

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
  
  static Console& Self();
};

#endif //#ifndef ZXTUNE123_CONSOLE_H_DEFINED
