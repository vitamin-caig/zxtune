/**
*
* @file
*
* @brief  Basic types definitions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <char_type.h>
//std includes
#include <cstdint>
#include <string>
#include <vector>

//@{
//! @brief Integer types
using std::int8_t;
using std::uint8_t;
using std::int16_t;
using std::uint16_t;
using std::int32_t;
using std::uint32_t;
using std::int64_t;
using std::uint64_t;

/// Unsigned integer type
typedef unsigned int uint_t;
/// Signed integer type
typedef signed int int_t;
//@}

/// String-related types

//! @brief %String type
typedef std::basic_string<Char> String;

//! @brief Helper for creating String from the array of chars
template<std::size_t D>
inline String FromCharArray(const char (&str)[D])
{
  //do not keep last zero symbol
  return String(str, str + D);
}

//! @brief Helper for creating String from ordinary std::string
inline String FromStdString(const std::string& str)
{
  return String(str.begin(), str.end());
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
static_assert(sizeof(uint_t) >= sizeof(uint32_t), "Invalid uint_t type");
static_assert(sizeof(int_t) >= sizeof(int32_t), "Invalid int_t type");
static_assert(sizeof(uint8_t) == 1, "Invalid uint8_t type");
static_assert(sizeof(int8_t) == 1, "Invalid int8_t type");
static_assert(sizeof(uint16_t) == 2, "Invalid uint16_t type");
static_assert(sizeof(int16_t) == 2, "Invalid int16_t type");
static_assert(sizeof(uint32_t) == 4, "Invalid uint32_t type");
static_assert(sizeof(int32_t) == 4, "Invalid int32_t type");
static_assert(sizeof(uint64_t) == 8, "Invalid uint64_t type");
static_assert(sizeof(int64_t) == 8, "Invalid int64_t type");
