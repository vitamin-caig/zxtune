/*
Abstract:
  String helper types definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __STRING_HELPERS_H_DEFINED__
#define __STRING_HELPERS_H_DEFINED__

#include <string_type.h>

#include <map>
#include <list>
#include <vector>
#include <sstream>

//common types
typedef std::basic_ostringstream<Char> OutStringStream;
typedef std::basic_istringstream<Char> InStringStream;
typedef std::map<String, String> StringMap;
typedef std::vector<String> StringArray;
typedef std::list<String> StringList;

#endif //__STRING_HELPERS_H_DEFINED__
