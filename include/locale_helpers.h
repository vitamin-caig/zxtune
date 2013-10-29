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

bool IsAlpha(Char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool IsAlNum(Char c)
{
  return std::isdigit(c) || IsAlpha(c);
}
