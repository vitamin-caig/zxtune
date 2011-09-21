/*
Abstract:
  TrDOS utils implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
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
    const char* const invalidSym = std::find_if(type, ArrayEnd(type), 
      !(boost::is_from_range('0', '9') || boost::is_from_range('A', 'Z') || boost::is_from_range('a', 'z')));
    fname += String(type, invalidSym);
    return fname;
  }
}
