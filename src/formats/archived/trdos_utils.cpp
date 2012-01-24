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
//common includes
#include <locale_helpers.h>

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3])
  {
    static const Char FORBIDDEN_SYMBOLS[] = {'\\', '/', '\?', '\0'};
    static const Char TRDOS_REPLACER('_');
    const String& strName = FromCharArray(name);
    String fname(OptimizeString(strName, TRDOS_REPLACER));
    std::replace_if(fname.begin(), fname.end(), boost::algorithm::is_any_of(FORBIDDEN_SYMBOLS), TRDOS_REPLACER);
    if (!IsAlNum(type[0]))
    {
      return fname;
    }
    fname += '.';
    const char* const invalidSym = std::find_if(type, ArrayEnd(type), std::not1(std::ptr_fun(&IsAlNum)));
    fname += String(type, invalidSym);
    return fname;
  }
}
