#ifndef __PLAYER_CONVERT_H_DEFINED__
#define __PLAYER_CONVERT_H_DEFINED__

#include <tools.h>

namespace ZXTune
{
  namespace Convert
  {
    struct Parameter
    {
      explicit Parameter(uint64_t id) : ID(id)
      {
      }
      const uint64_t ID;
    };
    
    
    template<uint8_t I1, uint8_t I2, uint8_t I3>
    struct ParamID
    {
      static const uint64_t Value = 
        uint64_t(I1) | (uint64_t(I2) << 8) | (uint64_t(I3) << 16);
    };
    
    template<uint8_t I1, uint8_t I2, uint8_t I3, uint8_t I4>
    struct ParamID
    {
      static const uint64_t Value = ParamID<I1, I2, I3>::Value | (uint64_t(I4) << 24);
    };
    
    template<uint8_t I1, uint8_t I2, uint8_t I3, uint8_t I4, uint8_t I5>
    struct ParamID
    {
      static const uint64_t Value = ParamID<I1, I2, I3, I4>::Value | (uint64_t(I5) << 32);
    };
    
    template<uint8_t I1, uint8_t I2, uint8_t I3, uint8_t I4, uint8_t I5, uint8_t I6>
    struct ParamID
    {
      static const uint64_t Value = ParamID<I1, I2, I3, I4, I5>::Value | (uint64_t(I6) << 40);
    };

    template<uint8_t I1, uint8_t I2, uint8_t I3, uint8_t I4, uint8_t I5, uint8_t I6, uint8_t I7>
    struct ParamID
    {
      static const uint64_t Value = ParamID<I1, I2, I3, I4, I5, I6>::Value | (uint64_t(I7) << 48);
    };

    template<uint8_t I1, uint8_t I2, uint8_t I3, uint8_t I4, uint8_t I5, uint8_t I6, uint8_t I7, uint8_t I8>
    struct ParamID
    {
      static const uint64_t Value = ParamID<I1, I2, I3, I4, I5, I6, I7>::Value | (uint64_t(I8) << 56);
    };

    /// Default parameter
    struct DummyParameter : public Parameter
    {
      static const uint64_t TYPE_ID = ParamID<'D', 'u', 'm', 'm', 'y'>::Value;
      DummyParameter() : Parameter(TYPE_ID)
      {
      }
    };
    /*
    struct OUTAYParameter : public Parameter
    {
      static const uint64_t TYPE_ID = ParamID<'O', 'U', 'T', 'A', 'Y'>::Value;
      OUTAYParameter() : Parameter(TYPE_ID)
      {
      }
      uint16_t PortSelect;
      uint16_t PortOutput;
    };
    
    struct OUTSoundriveParameter : public Parameter
    {
      static const uint64_t TYPE_ID = ParamID<'O', 'U', 'T', 'S', 'D'>::Value;
      uint16_t Ports[4];
    };
    
    struct PSGParameter : public Parameter
    {
      static const uint64_t TYPE_ID = ParamID<'P', 'S', 'G'>::Value;
      PSGParameter() : Parameter(TYPE_ID)
      {
      }
      bool Compress;
    };
    */
    
    template<class T>
    const T* parameter_cast(const Parameter* in)
    {
      return in.ID == T::TYPE_ID ? safe_ptr_cast<const T*>(in) : 0;
    }
  }
}

#endif //__PLAYER_CONVERT_H_DEFINED__
