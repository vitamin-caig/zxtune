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

//////// TRDOS-related

inline String GetTRDosName(const char (&name)[8], const char (&type)[3])
{
  static const Char FORBIDDEN_SYMBOLS[] = {'\\', '/', '\?', '\0'};
  static const Char TRDOS_REPLACER('_');
  String fname(OptimizeString(FromStdString(name), TRDOS_REPLACER));
  std::replace_if(fname.begin(), fname.end(), boost::algorithm::is_any_of(FORBIDDEN_SYMBOLS), TRDOS_REPLACER);
  return std::isalnum(type[0])
    ? fname + Char('.') + boost::algorithm::trim_right_copy_if(FromStdString(type), !boost::algorithm::is_alnum())
    : fname;
}

struct TRDFileEntry
{
  TRDFileEntry()
    : Name(), Offset(), Size()
  {
  }
  
  TRDFileEntry(const String& name, uint_t off, uint_t sz)
    : Name(name), Offset(off), Size(sz)
  {
  }
  
  bool IsMergeable(const TRDFileEntry& rh) const
  {
    //merge if files are sequental
    if (Offset + Size != rh.Offset)
    {
      return false;
    }
    //and previous file size is multiplication of 255 sectors
    if (0 == (Size % 0xff00))
    {
      return true;
    }
    //xxx.x and
    //xxx    '.x should be merged
    static const Char SATTELITE_SEQ[] = {'\'', '.', '\0'};
    const String::size_type apPos(rh.Name.find(SATTELITE_SEQ));
    return apPos != String::npos &&
      (Size == 0xc300 && rh.Size == 0xc000); //PDT sattelites sizes
  }

  void Merge(const TRDFileEntry& rh)
  {
    assert(IsMergeable(rh));
    Size += rh.Size;
  }
  
  String Name;
  uint_t Offset;
  uint_t Size;
};

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

#endif //__CORE_PLUGINS_PLAYERS_UTILS_H_DEFINED__
