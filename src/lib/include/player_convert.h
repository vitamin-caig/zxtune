#ifndef __PLAYER_CONVERT_H_DEFINED__
#define __PLAYER_CONVERT_H_DEFINED__

#include <tools.h>

namespace ZXTune
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
    
    inline uint64_t ParamID(uint8_t a, uint8_t b, uint8_t c)
    {
      return uint64_t(a) | (uint64_t(b) << 8) | (uint64_t(c) << 16);
    };
    
    inline uint64_t ParamID(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    {
      return ParamID(a, b, c) | (uint64_t(d) << 24);
    };
    
    inline uint64_t ParamID(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e)
    {
      return ParamID(a, b, c, d) | (uint64_t(e) << 32);
    };
    
    inline uint64_t ParamID(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f)
    {
      return ParamID(a, b, c, d, e) | (uint64_t(f) << 40);
    };

    inline uint64_t ParamID(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g)
    {
      return ParamID(a, b, c, d, e, f) | (uint64_t(g) << 48);
    };

    inline uint64_t ParamID(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h)
    {
      return ParamID(a, b, c, d, e, f, g) | (uint64_t(h) << 56);
    };

    /// Default parameter
    struct DummyParameter : public Parameter
    {
      static const uint64_t TYPE_ID = ParamID('D', 'u', 'm', 'm', 'y');
      DummyParameter() : Parameter(TYPE_ID)
      {
      }
    };

    template<class T>
    const T* parameter_cast(const Parameter* in)
    {
      return in->ID == T::TYPE_ID ? safe_ptr_cast<const T*>(in) : 0;
    }
  }
}

#endif //__PLAYER_CONVERT_H_DEFINED__
