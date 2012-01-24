/**
*
* @file     locale.h
* @brief    std::locale-independent symbol categories check functions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef LOCALE_H_DEFINED
#define LOCALE_H_DEFINED

#include <cctype>

bool IsAlpha(Char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool IsAlNum(Char c)
{
  return std::isdigit(c) || IsAlpha(c);
}

#endif //LOCALE_H_DEFINED
