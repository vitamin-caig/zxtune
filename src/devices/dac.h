/*
Abstract:
  DAC interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __DEVICES_DAC_H_DEFINED__
#define __DEVICES_DAC_H_DEFINED__

//common includes
#include <data_streaming.h>
#include <types.h>
//library includes
//TODO: remove
#include <sound/render_params.h>

//supporting for multichannel sample-based DAC
namespace Devices
{
  namespace DAC
  {
    struct DataChunk
    {
      struct ChannelData
      {
        ChannelData() : Channel(), Mask(), Enabled(), Note(), FreqSlideHz(), SampleNum()
        {
        }
        uint_t Channel;
        uint32_t Mask;

        enum
        {
          MASK_ENABLED = 1,
          MASK_NOTE = 2,
          MASK_NOTESLIDE = 4,
          MASK_FREQSLIDE = 8,
          MASK_SAMPLE = 16,
          MASK_POSITION = 32
        };

        bool Enabled;
        uint_t Note;
        int_t NoteSlide;
        int_t FreqSlideHz;
        uint_t SampleNum;
        uint_t PosInSample;
      };

      DataChunk() : Tick()
      {
      }
      uint64_t Tick;
      std::vector<ChannelData> Channels;
    };

    //channels state
    struct ChanState
    {
      ChanState()
        : Enabled(), Band(), LevelInPercents()
      {
      }

      //Is channel enabled to output
      bool Enabled;
      //Currently played tone band (up to 96)
      uint_t Band;
      //Currently played tone level percentage
      uint_t LevelInPercents;
    };
    typedef std::vector<ChanState> ChannelsState;

    class Chip
    {
    public:
      typedef boost::shared_ptr<Chip> Ptr;

      virtual ~Chip() {}

      /// Set sample for work
      virtual void SetSample(uint_t idx, const Dump& data, uint_t loop) = 0;

      /// render single data chunk
      virtual void RenderData(const ZXTune::Sound::RenderParameters& params,
                              const DataChunk& src) = 0;

      virtual void GetState(ChannelsState& state) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    // Sound is rendered in unsigned 16-bit values
    typedef uint16_t Sample;
    // Variable channels per sample
    typedef std::vector<Sample> MultiSample;
    // Result sound stream receiver
    typedef DataReceiver<MultiSample> Receiver;

    class ChipParameters
    {
    public:
      typedef boost::shared_ptr<const ChipParameters> Ptr;

      virtual ~ChipParameters() {}

      virtual uint_t ClockFreq() const = 0;
      virtual uint_t SoundFreq() const = 0;
      virtual bool Interpolate() const = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(uint_t channels, uint_t samples, uint_t sampleFreq, ChipParameters::Ptr params, Receiver::Ptr target);
  }
}

#endif //__DEVICES_DAC_H_DEFINED__
