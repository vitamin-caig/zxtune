#ifndef __AYM_H_DEFINED__
#define __AYM_H_DEFINED__

#include <types.h>
#include <sound.h>

#include "../data_source.h"

//supporting for AY/YM-based modules
namespace ZXTune
{
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
        // is beeper used
        BEEPER_MASK = 0x4000,
        // is beeper bit set (used only if BEEPER_MASK bit set)
        BEEPER_BIT = 0x8000,
      };
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

      /// renders sound values until target receive them
      virtual void RenderData(const Sound::Parameters& params,
                              DataSource<DataChunk>* src,
                              Sound::Receiver* dst) = 0;

      virtual void GetState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;

      /// Virtual constructors
      static Ptr Create();
    };
  }
}

#endif //__DEVICE_AYM_H_DEFINED__
