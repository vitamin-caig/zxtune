/**
*
* @file     string_helpers.h
* @brief    String helper types definitions
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __STRING_HELPERS_H_DEFINED__
#define __STRING_HELPERS_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <map>
#include <set>
#include <sstream>
#include <vector>

//common types
//! @brief ostringstream-based type for String
typedef std::basic_ostringstream<Char> OutStringStream;
//! @brief istringstream-based type for String
typedef std::basic_istringstream<Char> InStringStream;
//! @brief String => String map
typedef std::map<String, String> StringMap;
//! @brief Array of strings
typedef std::vector<String> StringArray;
//! @brief Set of strings
typedef std::set<String> StringSet;

#endif //__STRING_HELPERS_H_DEFINED__
