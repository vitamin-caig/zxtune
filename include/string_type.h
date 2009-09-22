/*
Abstract:
  Strings type definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __STRING_TYPE_H_DEFINED__
#define __STRING_TYPE_H_DEFINED__

#include <string>

#ifdef UNICODE
typedef std::wstring String;

template<std::size_t D>
inline String FromChar(const char (str&)[D])
{
  return String(str, str + D);
}
#else
typedef std::string String;

inline String FromChar(const char* str)
{
  return str;
}
#endif

#endif //__CHAR_TYPE_H_DEFINED__
