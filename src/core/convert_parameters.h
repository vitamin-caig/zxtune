/*
Abstract:
  Conversion support types

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_CONVERT_PARAMETERS_H_DEFINED__
#define __CORE_CONVERT_PARAMETERS_H_DEFINED__

#include "conversion.h"

namespace ZXTune
{
  namespace Module
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
}

#endif //__CORE_CONVERT_PARAMETERS_H_DEFINED__
