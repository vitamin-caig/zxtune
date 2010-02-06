/**
*
* @file     core/convert_parameters.h
* @brief    Conversion support types
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_CONVERT_PARAMETERS_H_DEFINED__
#define __CORE_CONVERT_PARAMETERS_H_DEFINED__

#include <core/conversion.h>

namespace ZXTune
{
  namespace Module
  {
    namespace Conversion
    {
      //! @brief %Parameter for conversion to raw format
      //! @see CAP_CONV_RAW
      struct RawConvertParam : public Parameter
      {
        static const uint64_t TYPE_ID = ParamID7<'R', 'a', 'w', 'D', 'a', 't', 'a'>::Value;
        RawConvertParam() : Parameter(TYPE_ID)
        {
        }
      };

      //! @brief %Parameter for conversion to PSG format
      //! @see CAP_CONV_PSG
      struct PSGConvertParam : public Parameter
      {
        static const uint64_t TYPE_ID = ParamID7<'P', 'S', 'G', 'D', 'a', 't', 'a'>::Value;
        PSGConvertParam() : Parameter(TYPE_ID)
        {
        }
      };

      //! @brief %Parameter for conversion to Vortex binary format
      //! @see CAP_CONV_VORTEX
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
