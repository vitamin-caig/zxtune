/*
Abstract:
  Basic types definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __TYPES_H_DEFINED__
#define __TYPES_H_DEFINED__

#include <string_type.h>

#include <map>
#include <list>
#include <vector>
#include <ostream>
#include <sstream>

#include <boost/format.hpp>
#include <boost/cstdint.hpp>

using boost::int8_t;
using boost::uint8_t;
using boost::int16_t;
using boost::uint16_t;
using boost::int32_t;
using boost::uint32_t;
using boost::int64_t;
using boost::uint64_t;

// alignment macroses
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

//common types
typedef std::basic_ostream<String::value_type> OutStream;
typedef std::basic_istream<String::value_type> InStream;
typedef std::basic_ostringstream<String::value_type> OutStringStream;
typedef std::basic_istringstream<String::value_type> InStringStream;
typedef std::map<String, String> StringMap;
typedef std::vector<String> StringArray;
typedef std::list<String> StringList;
typedef std::vector<uint8_t> Dump;
typedef boost::basic_format<String::value_type> Formatter;

#endif //__TYPES_H_DEFINED__
