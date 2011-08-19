/*
Abstract:
  TrDOS utils implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_catalogue.h"
#include "trdos_utils.h"
#include "core/plugins/utils.h"

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3])
  {
    static const Char FORBIDDEN_SYMBOLS[] = {'\\', '/', '\?', '\0'};
    static const Char TRDOS_REPLACER('_');
    const String& strName = FromCharArray(name);
    String fname(OptimizeString(strName, TRDOS_REPLACER));
    std::replace_if(fname.begin(), fname.end(), boost::algorithm::is_any_of(FORBIDDEN_SYMBOLS), TRDOS_REPLACER);
    if (!std::isalnum(type[0]))
    {
      return fname;
    }
    fname += '.';
    const char* const invalidSym = std::find_if(type, ArrayEnd(type), !boost::algorithm::is_alnum());
    fname += String(type, invalidSym);
    return fname;
  }

  bool AreFilesMergeable(const Container::File& lh, const Container::File& rh)
  {
    const std::size_t firstSize = lh.GetSize();
    //merge if files are sequental
    if (lh.GetOffset() + firstSize != rh.GetOffset())
    {
      return false;
    }
    //and previous file size is multiplication of 255 sectors
    if (0 == (firstSize % 0xff00))
    {
      return true;
    }
    //xxx.x and
    //xxx    '.x should be merged
    static const Char SATTELITE_SEQ[] = {'\'', '.', '\0'};
    const String::size_type apPos(rh.GetName().find(SATTELITE_SEQ));
    return apPos != String::npos &&
      (firstSize == 0xc300 && rh.GetSize() == 0xc000); //PDT sattelites sizes
  }
}
