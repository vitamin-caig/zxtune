/**
 *
 * @file
 *
 * @brief  Conversion support types
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "parameters.h"

namespace Module::Conversion
{
  // 0- none, 1- minimal, 2- maximal
  const uint_t DEFAULT_OPTIMIZATION = 1;

  //! @brief %Parameter for conversion to PSG format
  //! @see CAP_CONV_PSG
  struct PSGConvertParam : public Parameter
  {
    static const uint64_t TYPE_ID = ParamID7<'P', 'S', 'G', 'D', 'a', 't', 'a'>::Value;
    PSGConvertParam(uint_t opt = DEFAULT_OPTIMIZATION)
      : Parameter(TYPE_ID)
      , Optimization(opt)
    {}

    uint_t Optimization;
  };

  //! @brief %Parameter for conversion to ZX50 format
  //! @see CAP_CONV_ZX50
  struct ZX50ConvertParam : public Parameter
  {
    static const uint64_t TYPE_ID = ParamID8<'Z', 'X', '5', '0', 'D', 'a', 't', 'a'>::Value;
    ZX50ConvertParam(uint_t opt = DEFAULT_OPTIMIZATION)
      : Parameter(TYPE_ID)
      , Optimization(opt)
    {}

    uint_t Optimization;
  };

  //! @brief %Parameter for conversion to Vortex text format
  //! @see CAP_CONV_TXT
  struct TXTConvertParam : public Parameter
  {
    static const uint64_t TYPE_ID = ParamID8<'V', 'o', 'r', 't', 'T', 'e', 'x', 't'>::Value;
    TXTConvertParam()
      : Parameter(TYPE_ID)
    {}
  };

  //! @brief %Parameter for converting to debug stream output
  struct DebugAYConvertParam : public Parameter
  {
    static const uint64_t TYPE_ID = ParamID7<'D', 'e', 'b', 'u', 'g', 'A', 'Y'>::Value;
    DebugAYConvertParam(uint_t opt = DEFAULT_OPTIMIZATION)
      : Parameter(TYPE_ID)
      , Optimization(opt)
    {}

    uint_t Optimization;
  };

  //! @brief %Parameter for converting to raw ay stream format
  struct AYDumpConvertParam : public Parameter
  {
    static const uint64_t TYPE_ID = ParamID6<'A', 'Y', 'D', 'u', 'm', 'p'>::Value;
    AYDumpConvertParam(uint_t opt = DEFAULT_OPTIMIZATION)
      : Parameter(TYPE_ID)
      , Optimization(opt)
    {}

    uint_t Optimization;
  };

  //! @brief %Parameter for converting to FYM format
  struct FYMConvertParam : public Parameter
  {
    static const uint64_t TYPE_ID = ParamID3<'F', 'Y', 'M'>::Value;
    FYMConvertParam(uint_t opt = DEFAULT_OPTIMIZATION)
      : Parameter(TYPE_ID)
      , Optimization(opt)
    {}

    uint_t Optimization;
  };
}  // namespace Module::Conversion
