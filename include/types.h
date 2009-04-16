#ifndef __TYPES_H_DEFINED__
#define __TYPES_H_DEFINED__

#include <map>
#include <list>
#include <string>
#include <vector>
#include <ostream>

#ifdef _WIN32
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef signed int int32_t;
typedef unsigned uint32_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
#else
#include <cstdint>
#endif

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


//common types
typedef std::basic_ostringstream<String::value_type> OStringStream;
typedef std::map<String, String> StringMap;
typedef std::vector<String> StringArray;
typedef std::list<String> StringList;

typedef std::vector<uint8_t> Dump;

#endif //__TYPES_H_DEFINED__
