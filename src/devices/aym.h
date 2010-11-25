/*
Abstract:
  AY/YM chips interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __AYM_H_DEFINED__
#define __AYM_H_DEFINED__

//common includes
#include <types.h>
//library includes
#include <sound/receiver.h>
//std includes
#include <memory>

//supporting for AY/YM-based modules
namespace ZXTune
{
  //forward declaration
  namespace Sound
  {
    class RenderParameters;
  }
  
  namespace AYM
  {
    const uint_t CHANNELS = 3;
    const uint_t LOGICAL_CHANNELS = 4;
    
    struct DataChunk
    {
      //registers offsets in data
      enum
      {
        REG_TONEA_L,
        REG_TONEA_H,
        REG_TONEB_L,
        REG_TONEB_H,
        REG_TONEC_L,
        REG_TONEC_H,
        REG_TONEN,
        REG_MIXER,
        REG_VOLA,
        REG_VOLB,
        REG_VOLC,
        REG_TONEE_L,
        REG_TONEE_H,
        REG_ENV,

        //parameters offsets in data
        PARAM_DUTY_CYCLE,
        PARAM_DUTY_CYCLE_MASK,
        PARAM_LAYOUT,

        //limiter
        PARAM_LAST,
      };

      //masks
      enum
      {
        //to mark all registers actual
        MASK_ALL_REGISTERS = (1 << (REG_ENV + 1)) - 1,

        //only bits, no data
        //use YM chip
        YM_CHIP = 1 << (REG_ENV + 1),
        //use interpolation
        INTERPOLATE = YM_CHIP << 1,

        //bits in REG_VOL*
        REG_MASK_VOL = 0x0f,
        REG_MASK_ENV = 0x10,
        //bits in REG_MIXER
        REG_MASK_TONEA = 0x01,
        REG_MASK_NOISEA = 0x08,
        REG_MASK_TONEB = 0x02,
        REG_MASK_NOISEB = 0x10,
        REG_MASK_TONEC = 0x04,
        REG_MASK_NOISEC = 0x20,

        //bits in PARAM_DUTY_CYCLE_MASK
        DUTY_CYCLE_MASK_A = 1,
        DUTY_CYCLE_MASK_B = 2,
        DUTY_CYCLE_MASK_C = 4,
        DUTY_CYCLE_MASK_N = 8,
        DUTY_CYCLE_MASK_E = 16,
      };

      DataChunk() : Tick(), Mask(), Data()
      {
      }
      uint64_t Tick;
      uint_t Mask;
      boost::array<uint8_t, PARAM_LAST> Data;
    };

    //layout mode
    enum LayoutType
    {
      LAYOUT_ABC = 0,
      LAYOUT_ACB = 1,
      LAYOUT_BAC = 2,
      LAYOUT_BCA = 3,
      LAYOUT_CAB = 4,
      LAYOUT_CBA = 5,

      LAYOUT_LAST
    };

    //channels state
    struct ChanState
    {
      ChanState()
        : Name(' '), Enabled(), Band(), LevelInPercents()
      {
      }

      explicit ChanState(Char name)
        : Name(name), Enabled(), Band(), LevelInPercents()
      {
      }
        
      //Short channel abbreviation
      Char Name;
      //Is channel enabled to output
      bool Enabled;
      //Currently played tone band (up to 96)
      uint_t Band;
      //Currently played tone level percentage
      uint_t LevelInPercents;
    };
    typedef boost::array<ChanState, LOGICAL_CHANNELS> ChannelsState;

    class Chip
    {
    public:
      typedef std::auto_ptr<Chip> Ptr;

      virtual ~Chip() {}

      /// render single data chunk
      virtual void RenderData(const Sound::RenderParameters& params,
                              const DataChunk& src,
                              Sound::MultichannelReceiver& dst) = 0;

      virtual void GetState(ChannelsState& state) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip();
    Chip::Ptr CreatePSGDumper(Dump& data);
    Chip::Ptr CreateZX50Dumper(Dump& data);
  }
}

#endif //__DEVICE_AYM_H_DEFINED__
