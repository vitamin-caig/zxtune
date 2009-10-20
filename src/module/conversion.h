/*
Abstract:
  Modules types definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __MODULE_CONVERSION_H_DEFINED__
#define __MODULE_CONVERSION_H_DEFINED__

#include <tools.h>

namespace ZXTune
{
  namespace Module
  {
    namespace Conversion
    {
      struct Parameter
      {
        explicit Parameter(uint64_t id) : ID(id)
        {
        }
        const uint64_t ID;
      };
      
      template<uint8_t a, uint8_t b, uint8_t c>
      struct ParamID3
      {
        static const uint64_t Value = uint64_t(a) | (uint64_t(b) << 8) | (uint64_t(c) << 16);
      };
      
      template<uint8_t a, uint8_t b, uint8_t c, uint8_t d>
      struct ParamID4
      {
        static const uint64_t Value = ParamID3<a, b, c>::Value | (uint64_t(d) << 24);
      };
      
      template<uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e>
      struct ParamID5
      {
        static const uint64_t Value = ParamID4<a, b, c, d>::Value | (uint64_t(e) << 32);
      };
      
      template<uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f>
      struct ParamID6
      {
        static const uint64_t Value = ParamID5<a, b, c, d, e>::Value | (uint64_t(f) << 40);
      };

      template<uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g>
      struct ParamID7
      {
        static const uint64_t Value = ParamID6<a, b, c, d, e, f>::Value | (uint64_t(g) << 48);
      };

      template<uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h>
      struct ParamID8
      {
        static const uint64_t Value = ParamID7<a, b, c, d, e, f, g>::Value | (uint64_t(h) << 56);
      };

      /// Default parameter
      struct DummyParameter : public Parameter
      {
        static const uint64_t TYPE_ID = ParamID5<'D', 'u', 'm', 'm', 'y'>::Value;
        DummyParameter() : Parameter(TYPE_ID)
        {
        }
      };

      /// Used to determine real parameters type
      template<class T>
      const T* parameter_cast(const Parameter* in)
      {
        return in->ID == T::TYPE_ID ? safe_ptr_cast<const T*>(in) : 0;
      }
    }
  }
}

#endif //__MODULE_CONVERSION_H_DEFINED__
