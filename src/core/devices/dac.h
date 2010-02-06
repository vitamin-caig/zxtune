/*
Abstract:
  DAC interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __DAC_H_DEFINED__
#define __DAC_H_DEFINED__

#include <types.h>

#include <core/module_types.h>
#include <sound/receiver.h>

#include <memory>

//supporting for multichannel sample-based DAC
namespace ZXTune
{
  //forward declarations
  namespace Sound
  {
    struct RenderParameters;
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

    class Chip
    {
    public:
      typedef std::auto_ptr<Chip> Ptr;

      virtual ~Chip()
      {
      }

      /// Set sample for work
      virtual void SetSample(uint_t idx, const Dump& data, uint_t loop) = 0;

      /// render single data chunk
      virtual void RenderData(const Sound::RenderParameters& params,
                              const DataChunk& src,
                              Sound::MultichannelReceiver& dst) = 0;

      virtual void GetState(Module::Analyze::ChannelsState& state) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(uint_t channels, uint_t samples, uint_t sampleFreq);
  }
}

#endif //__DEVICE_AYM_H_DEFINED__
