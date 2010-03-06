/**
*
* @file     types.h
* @brief    Basic types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __TYPES_H_DEFINED__
#define __TYPES_H_DEFINED__

#include <char_type.h>

#include <string>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>

//@{
//! @brief Integer types
using boost::int8_t;
using boost::uint8_t;
using boost::int16_t;
using boost::uint16_t;
// int32_t/uint32_t are already defined by environment
using boost::int64_t;
using boost::uint64_t;

/// Use unsigned memtype as unsigned integer
typedef std::size_t uint_t;
/// Use signed memtype as signed integer
typedef std::ptrdiff_t int_t;
//@}

/// String-related types

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

//@{
//! Structure packing macros
//! @code
//! #ifdef USE_PRAGMA_PACK
//! #pragma pack(push,1)
//! #endif
//! PACK_PRE struct Foo
//! {
//! ...
//! } PACK_POST;
//!
//! PACK_PRE struct Bar
//! {
//! ...
//! } PACK_POST;
//! #ifdef USE_PRAGMA_PACK
//! #pragma pack(pop)
//! #endif
//! @endcode
#if defined __GNUC__
#define PACK_PRE
#define PACK_POST __attribute__ ((packed))
#elif defined _MSC_VER
#define PACK_PRE
#define PACK_POST
#define USE_PRAGMA_PACK
#else
#define PACK_PRE
#define PACK_POST
#endif
//@}

//! @brief Plain data type
typedef std::vector<uint8_t> Dump;

//assertions
BOOST_STATIC_ASSERT(sizeof(uint_t) >= sizeof(uint32_t));
BOOST_STATIC_ASSERT(sizeof(int_t) >= sizeof(int32_t));

#endif //__TYPES_H_DEFINED__
