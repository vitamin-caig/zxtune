/**
*
* @file
*
* @brief  std::locale-independent symbol categories check functions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

#include <cctype>

inline bool IsAlpha(Char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline bool IsDigit(Char c)
{
  return c >= '0' && c <= '9';
}

inline bool IsAlNum(Char c)
{
  return IsDigit(c) || IsAlpha(c);
}
