/*
Abstract:
  Different utils

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_UTILS_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_UTILS_H_DEFINED__

#include <algorithm>
#include <cctype>

#include <boost/algorithm/string/trim.hpp>

inline String OptimizeString(const String& str)
{
  String res(boost::algorithm::trim_copy_if(str, boost::algorithm::is_cntrl() || boost::algorithm::is_space()));
  std::replace_if(res.begin(), res.end(), std::ptr_fun<int, int>(&std::iscntrl), '\?');
  return res;
}

#endif //__CORE_PLUGINS_PLAYERS_UTILS_H_DEFINED__

