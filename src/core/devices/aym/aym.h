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

#include <types.h>

#include <core/module_types.h>

//supporting for AY/YM-based modules
namespace ZXTune
{
  //forward declaration
  namespace Sound
  {
    class MultichannelReceiver;
    struct RenderParameters;
  }
  
  namespace AYM
  {
    struct DataChunk
    {
      enum
      {
        //registers offsets in data
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

        //bits in REG_VOL*
        MASK_VOL = 0x0f,
        MASK_ENV = 0x10,
        //bits in REG_MIXER
        MASK_TONEA = 0x01,
        MASK_NOISEA = 0x08,
        MASK_TONEB = 0x02,
        MASK_NOISEB = 0x10,
        MASK_TONEC = 0x04,
        MASK_NOISEC = 0x20,

        //to mark all registers actual
        ALL_REGISTERS = 0x3fff,
      };

      DataChunk() : Tick(), Mask()
      {
      }
      uint64_t Tick;
      uint16_t Mask;
      uint8_t Data[14];
    }; //24 bytes total

    class Chip
    {
    public:
      typedef std::auto_ptr<Chip> Ptr;

      virtual ~Chip()
      {
      }

      /// render single data chunkg
      virtual void RenderData(const Sound::RenderParameters& params,
                              const DataChunk& src,
                              Sound::MultichannelReceiver& dst) = 0;

      virtual void GetState(Module::Analyze::ChannelsState& state) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip();
    Chip::Ptr CreatePSGDumper();
    Chip::Ptr CreateOUTDumper();
  }
}

#endif //__DEVICE_AYM_H_DEFINED__
