/**
 *
 * @file
 *
 * @brief  Conversion base support types
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "pointers.h"
#include "types.h"

namespace Module::Conversion
{
  //! @brief Base conversion parameter structure
  struct Parameter
  {
    explicit Parameter(uint64_t id)
      : ID(id)
    {}

    virtual ~Parameter() = default;

    //! Conversion target id
    const uint64_t ID;
  };

  //! @brief Three-letters identifier helper
  template<uint8_t a, uint8_t b, uint8_t c>
  struct ParamID3
  {
    static const uint64_t Value = uint64_t(a) | (uint64_t(b) << 8) | (uint64_t(c) << 16);
  };

  //! @brief Four-letters identifier helper
  template<uint8_t a, uint8_t b, uint8_t c, uint8_t d>
  struct ParamID4
  {
    static const uint64_t Value = ParamID3<a, b, c>::Value | (uint64_t(d) << 24);
  };

  //! @brief Five-letters identifier helper
  template<uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e>
  struct ParamID5
  {
    static const uint64_t Value = ParamID4<a, b, c, d>::Value | (uint64_t(e) << 32);
  };

  //! @brief Six-letters identifier helper
  template<uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f>
  struct ParamID6
  {
    static const uint64_t Value = ParamID5<a, b, c, d, e>::Value | (uint64_t(f) << 40);
  };

  //! @brief Seven-letters identifier helper
  template<uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g>
  struct ParamID7
  {
    static const uint64_t Value = ParamID6<a, b, c, d, e, f>::Value | (uint64_t(g) << 48);
  };

  //! @brief Eight-letters identifier helper
  template<uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h>
  struct ParamID8
  {
    static const uint64_t Value = ParamID7<a, b, c, d, e, f, g>::Value | (uint64_t(h) << 56);
  };

  //! @brief Dummy parameter type
  struct DummyParameter : public Parameter
  {
    static const uint64_t TYPE_ID = ParamID5<'D', 'u', 'm', 'm', 'y'>::Value;
    DummyParameter()
      : Parameter(TYPE_ID)
    {}
  };

  //! @brief Determine real parameters type
  template<class T>
  const T* parameter_cast(const Parameter* in)
  {
    return in->ID == T::TYPE_ID ? safe_ptr_cast<const T*>(in) : nullptr;
  }

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
