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
#include <logging.h>
#include <tools.h>
//library includes
#include <core/module_detect.h>
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

inline Log::ProgressCallback::Ptr CreateProgressCallback(const ZXTune::DetectParameters& params, uint_t limit)
{
  if (Log::ProgressCallback* cb = params.GetProgressCallback())
  {
    return Log::CreatePercentProgressCallback(limit, *cb);
  }
  return Log::ProgressCallback::Ptr();
}

#endif //__CORE_PLUGINS_UTILS_H_DEFINED__
