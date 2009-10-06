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

#include <char_type.h>

#include <map>
#include <list>
#include <string>
#include <vector>
#include <ostream>
#include <sstream>

#include <boost/format.hpp>
#include <boost/cstdint.hpp>
#include <boost/variant.hpp>

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
typedef std::basic_string<Char> String;
typedef std::basic_ostream<Char> OutStream;
typedef std::basic_istream<Char> InStream;
typedef std::basic_ostringstream<Char> OutStringStream;
typedef std::basic_istringstream<Char> InStringStream;
typedef std::map<String, String> StringMap;
typedef std::vector<String> StringArray;
typedef std::list<String> StringList;
typedef std::vector<uint8_t> Dump;
typedef boost::basic_format<Char> Formatter;

//specific types
typedef boost::variant<int64_t, String, Dump> CommonParameter;
typedef std::map<String, CommonParameter> ParametersMap;

#endif //__TYPES_H_DEFINED__
