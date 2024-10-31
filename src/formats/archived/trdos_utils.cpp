/**
 *
 * @file
 *
 * @brief  TR-DOS utilities
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/archived/trdos_utils.h"

#include <strings/encoding.h>
#include <strings/optimize.h>
#include <tools/locale_helpers.h>

#include <string_view.h>

#include <algorithm>

namespace TRDos
{
  String GetEntryName(const char (&name)[8], const char (&type)[3])
  {
    const auto TRDOS_REPLACER = '_';
    String fname = Strings::OptimizeAscii(StringView(name, sizeof(name)), TRDOS_REPLACER);
    std::replace_if(
        fname.begin(), fname.end(), [](auto c) { return c == '\\' || c == '/' || c == '\?'; }, TRDOS_REPLACER);
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
