#ifndef __TRDOS_H_DEFINED__
#define __TRDOS_H_DEFINED__

#include <tools.h>

#include <cctype>
#include <algorithm>
#include <functional>

namespace ZXTune
{
  namespace IO
  {
    struct TRDosName
    {
      uint8_t Filename[8];
      uint8_t Filetype[3];
    };

    inline bool IsFilenameSym(String::value_type sym)
    {
      return std::isprint(sym) && sym != '?' && sym != '/' && sym != '\\';
    }

    inline String GetFileName(const TRDosName& trdos)
    {
      const String& name = String(trdos.Filename, ArrayEnd(trdos.Filename));
      String result = name.substr(0, name.find_last_not_of(' ') + 1) + '.' +
        String(trdos.Filetype, std::find_if(trdos.Filetype, ArrayEnd(trdos.Filetype),
        std::not1(std::ptr_fun(&isalnum))));
      std::replace_if(result.begin(), result.end(), std::not1(std::ptr_fun(IsFilenameSym)), '_');
      return result;
    }
  }
}

#endif //__TRDOS_H_DEFINED__
