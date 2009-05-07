#ifndef __TYPES_H_DEFINED__
#define __TYPES_H_DEFINED__

#include <map>
#include <list>
#include <string>
#include <vector>
#include <ostream>
#include <sstream>

#include <boost/cstdint.hpp>
#include <boost/detail/endian.hpp>

using boost::int8_t;
using boost::uint8_t;
using boost::int16_t;
using boost::uint16_t;
using boost::int32_t;
using boost::uint32_t;
using boost::int64_t;
using boost::uint64_t;

#ifdef UNICODE

typedef std::wstring String;
typedef std::wostream OutStream;

#else

typedef std::string String;
typedef std::ostream OutStream;

#endif

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

//byte order macroses

inline uint16_t swapBytes(uint16_t a)
{
  return ((a & 0xff) << 8) | ((a & 0xff) >> 8);
}

#ifdef BOOST_LITTLE_ENDIAN
template<class T>
inline T fromLE(T a)
{
  return a;
}

template<class T>
inline T fromBE(T a)
{
  return swapBytes(a);
}

#elif defined(BOOST_BIG_ENDIAN)
template<class T>
inline T fromLE(T a)
{
  return swapBytes(a);
}

template<class T>
inline T fromBE(T a)
{
  return a;
}
#else
#error Invalid byte order
#endif

//common types
typedef std::basic_ostringstream<String::value_type> OutStringStream;
typedef std::basic_istringstream<String::value_type> InStringStream;
typedef std::map<String, String> StringMap;
typedef std::vector<String> StringArray;
typedef std::list<String> StringList;
typedef std::vector<uint8_t> Dump;

#endif //__TYPES_H_DEFINED__
