#ifndef __CONVERT_PARAMETERS_H_DEFINED__
#define __CONVERT_PARAMETERS_H_DEFINED__

#include "player_convert.h"

namespace ZXTune
{
  namespace Conversion
  {
    struct RawConvertParam : public Parameter
    {
      static const uint64_t TYPE_ID = ParamID7<'R', 'a', 'w', 'D', 'a', 't', 'a'>::Value;
      RawConvertParam() : Parameter(TYPE_ID)
      {
      }
    };

    struct VortexTextParam : public Parameter
    {
      static const uint64_t TYPE_ID = ParamID8<'V', 'o', 'r', 't', 'T', 'e', 'x', 't'>::Value;
      VortexTextParam() : Parameter(TYPE_ID)
      {
      }
    };
  }
}


#endif //__CONVERT_PARAMETERS_H_DEFINED__
