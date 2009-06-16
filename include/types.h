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
  return (a << 8) | (a >> 8);
}

inline uint32_t swapBytes(uint32_t a)
{
  const uint32_t tmp((((a >> 8) ^ (a << 8)) & 0xff00ff) ^ (a << 8));
  return (tmp << 16) | (tmp >> 16);
}

inline uint64_t swapBytes(uint64_t a)
{
  union H1
  {
    uint64_t Qword;
    struct H2
    {
      uint32_t Dword[2];
    } Dwords;
  } Helper;
  Helper.Dwords.Dword[0] = swapBytes(uint32_t(a >> 32));
  Helper.Dwords.Dword[1] = swapBytes(uint32_t(a));
  return Helper.Qword;
}

#ifdef BOOST_LITTLE_ENDIAN
inline bool isLE()
{
  return true;
}

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
inline bool isLE()
{
  return false;
}

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
