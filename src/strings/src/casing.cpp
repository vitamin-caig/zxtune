/**
 *
 * @file
 *
 * @brief  Casing-related implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <strings/casing.h>

#include <string_view.h>

#include <algorithm>
#include <cctype>

namespace Strings
{
  String ToLowerAscii(StringView str)
  {
    String res(str.size(), ' ');
    std::transform(str.begin(), str.end(), res.begin(), [](unsigned char sym) { return std::tolower(sym); });
    return res;
  }

  String ToUpperAscii(StringView str)
  {
    String res(str.size(), ' ');
    std::transform(str.begin(), str.end(), res.begin(), [](unsigned char sym) { return std::toupper(sym); });
    return res;
  }

  bool EqualNoCaseAscii(StringView lh, StringView rh)
  {
    if (lh.size() != rh.size())
    {
      return false;
    }
    for (std::size_t i = 0, lim = lh.size(); i != lim; ++i)
    {
      if (0 != ((lh[i] ^ rh[i]) & ~('A' ^ 'a')))
      {
        return false;
      }
    }
    return true;
  }
}  // namespace Strings
