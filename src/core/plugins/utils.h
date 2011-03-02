/*
Abstract:
  Different utils

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_UTILS_H_DEFINED__
#define __CORE_PLUGINS_UTILS_H_DEFINED__

//common includes
#include <tools.h>
//std includes
#include <algorithm>
#include <cctype>
//boost includes
#include <boost/algorithm/string/trim.hpp>

inline String OptimizeString(const String& str, Char replace = '\?')
{
  // applicable only printable chars in range 0x21..0x7f
  String res(boost::algorithm::trim_copy_if(str, !boost::is_from_range('\x21', '\x7f')));
  std::replace_if(res.begin(), res.end(), std::ptr_fun<int, int>(&std::iscntrl), replace);
  return res;
}

#endif //__CORE_PLUGINS_UTILS_H_DEFINED__
