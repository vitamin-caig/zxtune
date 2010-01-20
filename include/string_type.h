/**
*
* @file string_type.h
* @brief String type definition
* @version $Id$
* @author (C) Vitamin/CAIG/2001
*
**/

#ifndef __STRING_TYPE_H_DEFINED__
#define __STRING_TYPE_H_DEFINED__

#include <char_type.h>

#include <string>

//! @brief %String type
typedef std::basic_string<Char> String;

//! @brief Helper for creating String from the array of chars
template<std::size_t D>
inline String FromChar(const char (&str)[D])
{
  return String(str, str + D);
}

#endif //__STRING_H_DEFINED__
