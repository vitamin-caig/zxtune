/**
*
* @file     core/convert_parameters.h
* @brief    Conversion support types
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __CORE_CONVERT_PARAMETERS_H_DEFINED__
#define __CORE_CONVERT_PARAMETERS_H_DEFINED__

//library includes
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

      //! @brief %Parameter for conversion to ZX50 format
      //! @see CAP_CONV_ZX50
      struct ZX50ConvertParam : public Parameter
      {
        static const uint64_t TYPE_ID = ParamID8<'Z', 'X', '5', '0', 'D', 'a', 't', 'a'>::Value;
        ZX50ConvertParam() : Parameter(TYPE_ID)
        {
        }
      };

      //! @brief %Parameter for conversion to Vortex text format
      //! @see CAP_CONV_TXT
      struct TXTConvertParam : public Parameter
      {
        static const uint64_t TYPE_ID = ParamID8<'V', 'o', 'r', 't', 'T', 'e', 'x', 't'>::Value;
        TXTConvertParam() : Parameter(TYPE_ID)
        {
        }
      };

      //! @brief %Parameter for converting to debug stream output
      struct DebugAYConvertParam : public Parameter
      {
        static const uint64_t TYPE_ID = ParamID7<'D', 'e', 'b', 'u', 'g', 'A', 'Y'>::Value;
        DebugAYConvertParam() : Parameter(TYPE_ID)
        {
        }
      };
      
      //! @brief %Parameter for converting to raw ay stream format
      struct AYDumpConvertParam : public Parameter
      {
        static const uint64_t TYPE_ID = ParamID6<'A', 'Y', 'D', 'u', 'm', 'p'>::Value;
        AYDumpConvertParam() : Parameter(TYPE_ID)
        {
        }
      };

      //! @brief %Parameter for converting to FYM format
      struct FYMConvertParam : public Parameter
      {
        static const uint64_t TYPE_ID = ParamID3<'F', 'Y', 'M'>::Value;
        FYMConvertParam() : Parameter(TYPE_ID)
        {
        }
      };
    }
  }
}

#endif //__CORE_CONVERT_PARAMETERS_H_DEFINED__
