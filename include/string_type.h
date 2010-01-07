/*
Abstract:
  String type definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __STRING_TYPE_H_DEFINED__
#define __STRING_TYPE_H_DEFINED__

#include <char_type.h>

#include <string>

typedef std::basic_string<Char> String;

template<std::size_t D>
inline String FromChar(const Char (&str)[D])
{
  return String(str, str + D);
}

#endif //__STRING_H_DEFINED__
