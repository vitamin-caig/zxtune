/*
Abstract:
  AY/YM chips interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __DEVICES_AYM_H_DEFINED__
#define __DEVICES_AYM_H_DEFINED__

//common includes
#include <data_streaming.h>
#include <types.h>
//boost includes
#include <boost/array.hpp>

//supporting for AY/YM-based modules
namespace Devices
{
  namespace AYM
  {
    const uint_t CHANNELS = 3;
    const uint_t VOICES = 5;

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

        //limiter
        REG_LAST,
      };

      //masks
      enum
      {
        //to mark all registers actual
        MASK_ALL_REGISTERS = (1 << REG_LAST) - 1,

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
      boost::array<uint8_t, REG_LAST> Data;
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
    typedef boost::array<ChanState, VOICES> ChannelsState;

    class Chip
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;

      virtual ~Chip() {}

      /// render single data chunk
      virtual void RenderData(const DataChunk& src) = 0;

      virtual void GetState(ChannelsState& state) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    // Sound is rendered in unsigned 16-bit values
    typedef uint16_t Sample;
    // 3 channels per sample
    typedef boost::array<Sample, CHANNELS> MultiSample;
    // Result sound stream receiver
    typedef DataReceiver<MultiSample> Receiver;

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;

      virtual ~ChipParameters() {}

      virtual uint_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
      virtual bool IsYM() const = 0;
      virtual bool Interpolate() const = 0;
      virtual uint_t DutyCycleValue() const = 0;
      virtual uint_t DutyCycleMask() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Receiver::Ptr target);
    Chip::Ptr CreatePSGDumper(uint_t clocksPerFrame, Dump& data);
    Chip::Ptr CreateZX50Dumper(uint_t clocksPerFrame, Dump& data);
    Chip::Ptr CreateDebugDumper(uint_t clocksPerFrame, Dump& data);
    Chip::Ptr CreateRawStreamDumper(uint_t clocksPerFrame, Dump& data);
  }
}

#endif //__DEVICES_AYM_H_DEFINED__
