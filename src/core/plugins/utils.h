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

/////// Packing-related

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
PACK_PRE struct Hrust2xHeader
{
  uint8_t LastBytes[6];
  uint8_t FirstByte;
  uint8_t BitStream[1];
} PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

BOOST_STATIC_ASSERT(sizeof(Hrust2xHeader) == 8);

// implemented in hrust2x plugin, size is packed, dst is not cleared
bool DecodeHrust2x(const Hrust2xHeader* header, uint_t size, Dump& dst);

#endif //__CORE_PLUGINS_UTILS_H_DEFINED__
