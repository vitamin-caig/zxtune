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
inline String FromStdString(const char (&str)[D])
{
  return String(str, str + D);
}

//! @brief Helper for creating String from ordinary std::string
inline String FromStdString(const std::string& str)
{
  return String(str.begin(), str.end());
}

//! @brief Helper for creating ordinary std::string from the array of Chars
template<std::size_t D>
inline std::string ToStdString(const Char (&str)[D])
{
  return std::string(str, str + D);
}

//! @brief Helper for creating ordinary std::string from the String
inline std::string ToStdString(const String& str)
{
  return std::string(str.begin(), str.end());
}

#endif //__STRING_H_DEFINED__
