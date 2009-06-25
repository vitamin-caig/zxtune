#ifndef __CONVERT_PARAMETERS_H_DEFINED__
#define __CONVERT_PARAMETERS_H_DEFINED__

#include "player_convert.h"

namespace ZXTune
{
  namespace Conversion
  {
    struct RawConvertParam : public Parameter
    {
      static const uint64_t TYPE_ID = ParamID('R', 'a', 'w', 'D', 'a', 't', 'a');
      RawConvertParam() : Parameter(TYPE_ID)
      {
      }
    };
  }
}


#endif //__CONVERT_PARAMETERS_H_DEFINED__
