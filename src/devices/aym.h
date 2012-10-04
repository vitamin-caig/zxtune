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
//library includes
#include <time/stamp.h>
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
        //Due to performance issues, lets AY emulate also beeper.
        //Register is like a volume registers. If it's not zero, it overrides all channels output
        REG_BEEPER,
        //limiter
        REG_LAST_AY = REG_ENV + 1,
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
      };

      enum
      {
        CHANNEL_MASK_A = 1,
        CHANNEL_MASK_B = 2,
        CHANNEL_MASK_C = 4,
        CHANNEL_MASK_N = 8,
        CHANNEL_MASK_E = 16,
      };

      DataChunk() : TimeStamp(), Mask(), Data()
      {
      }
      Time::Nanoseconds TimeStamp;
      uint32_t Mask;
      boost::array<uint8_t, REG_LAST> Data;
    };

    enum LayoutType
    {
      LAYOUT_ABC = 0,
      LAYOUT_ACB = 1,
      LAYOUT_BAC = 2,
      LAYOUT_BCA = 3,
      LAYOUT_CAB = 4,
      LAYOUT_CBA = 5,
      LAYOUT_MONO = 6,

      LAYOUT_LAST
    };

    class Device
    {
    public:
      typedef boost::shared_ptr<Device> Ptr;
      virtual ~Device() {}

      /// render single data chunk
      virtual void RenderData(const DataChunk& src) = 0;

      /// flush any collected data
      virtual void Flush() = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
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

    // Describes real device
    class Chip : public Device
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;

      virtual void GetState(ChannelsState& state) const = 0;
    };

    // Describes dump converter
    class Dumper : public Device
    {
    public:
      typedef boost::shared_ptr<Dumper> Ptr;

      virtual void GetDump(Dump& result) const = 0;
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

      virtual uint64_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
      virtual bool IsYM() const = 0;
      virtual bool Interpolate() const = 0;
      virtual uint_t DutyCycleValue() const = 0;
      virtual uint_t DutyCycleMask() const = 0;
      virtual LayoutType Layout() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Receiver::Ptr target);

    class DumperParameters
    {
    public:
      typedef boost::shared_ptr<const DumperParameters> Ptr;
      virtual ~DumperParameters() {}

      enum Optimization
      {
        NONE,
        NORMAL,
        MAXIMUM
      };

      virtual Time::Microseconds FrameDuration() const = 0;
      virtual Optimization OptimizationLevel() const = 0;
    };

    Dumper::Ptr CreatePSGDumper(DumperParameters::Ptr params);
    Dumper::Ptr CreateZX50Dumper(DumperParameters::Ptr params);
    Dumper::Ptr CreateDebugDumper(DumperParameters::Ptr params);
    Dumper::Ptr CreateRawStreamDumper(DumperParameters::Ptr params);

    class FYMDumperParameters : public DumperParameters
    {
    public:
      typedef boost::shared_ptr<const FYMDumperParameters> Ptr;

      virtual uint64_t ClockFreq() const = 0;
      virtual String Title() const = 0;
      virtual String Author() const = 0;
      virtual uint_t LoopFrame() const = 0;
    };
    Dumper::Ptr CreateFYMDumper(FYMDumperParameters::Ptr params);
  }
}

#endif //__DEVICES_AYM_H_DEFINED__
