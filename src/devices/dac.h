/*
Abstract:
  DAC interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __DAC_H_DEFINED__
#define __DAC_H_DEFINED__

//common includes
#include <types.h>
//library includes
#include <sound/receiver.h>
//std includes
#include <memory>

//supporting for multichannel sample-based DAC
namespace ZXTune
{
  //forward declarations
  namespace Sound
  {
    class RenderParameters;
  }

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

      DataChunk() : Tick(), Interpolate()
      {
      }
      uint64_t Tick;
      bool Interpolate;
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
      typedef std::auto_ptr<Chip> Ptr;

      virtual ~Chip() {}

      /// Set sample for work
      virtual void SetSample(uint_t idx, const Dump& data, uint_t loop) = 0;

      /// render single data chunk
      virtual void RenderData(const Sound::RenderParameters& params,
                              const DataChunk& src) = 0;

      virtual void GetState(ChannelsState& state) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(uint_t channels, uint_t samples, uint_t sampleFreq, Sound::MultichannelReceiver::Ptr target);
  }
}

#endif //__DEVICE_AYM_H_DEFINED__
