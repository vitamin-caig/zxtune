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

inline String OptimizeString(const String& str, Char replace = '\?')
{
  String res(boost::algorithm::trim_copy_if(str, boost::algorithm::is_cntrl() || boost::algorithm::is_space()));
  std::replace_if(res.begin(), res.end(), std::ptr_fun<int, int>(&std::iscntrl), replace);
  return res;
}

inline String GetTRDosName(const char (&name)[8], const char (&type)[3])
{
  static const Char TRDOS_REPLACER('_');
  const String& fname(OptimizeString(FromStdString(name), TRDOS_REPLACER));
  return std::isalnum(type[0])
    ? fname + Char('.') + boost::algorithm::trim_right_copy_if(FromStdString(type), !boost::algorithm::is_alnum())
    : fname;
}

#endif //__CORE_PLUGINS_PLAYERS_UTILS_H_DEFINED__

