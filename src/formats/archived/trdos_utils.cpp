/**
 *
 * @file
 *
 * @brief  TR-DOS utilities
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/trdos_utils.h"
// common includes
#include <locale_helpers.h>
// library includes
#include <strings/encoding.h>
#include <strings/optimize.h>
// std includes
#include <algorithm>
// boost includes
#include <boost/algorithm/string/classification.hpp>

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3])
  {
    static const Char FORBIDDEN_SYMBOLS[] = {'\\', '/', '\?', '\0'};
    static const Char TRDOS_REPLACER('_');
    String fname = Strings::OptimizeAscii(StringView(name, sizeof(name)), TRDOS_REPLACER);
    std::replace_if(fname.begin(), fname.end(), boost::algorithm::is_any_of(FORBIDDEN_SYMBOLS), TRDOS_REPLACER);
    if (IsAlNum(type[0]))
    {
      fname += '.';
      const char* const invalidSym = std::find_if_not(type, std::end(type), &IsAlNum);
      ;
      fname += String(type, invalidSym);
    }
    return Strings::ToAutoUtf8(fname);
  }
}  // namespace TRDos
