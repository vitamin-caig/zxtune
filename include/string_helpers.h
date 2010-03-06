/**
*
* @file     string_helpers.h
* @brief    String helper types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __STRING_HELPERS_H_DEFINED__
#define __STRING_HELPERS_H_DEFINED__

#include <types.h>

#include <map>
#include <set>
#include <list>
#include <vector>
#include <sstream>

//common types
//! @brief ostringstream-based type for String
typedef std::basic_ostringstream<Char> OutStringStream;
//! @brief istringstream-based type for String
typedef std::basic_istringstream<Char> InStringStream;
//! @brief String => String map
typedef std::map<String, String> StringMap;
//! @brief Set of strings
typedef std::set<String> StringSet;
//! @brief Array of strings
typedef std::vector<String> StringArray;
//! @brief List of strings
typedef std::list<String> StringList;

#endif //__STRING_HELPERS_H_DEFINED__
