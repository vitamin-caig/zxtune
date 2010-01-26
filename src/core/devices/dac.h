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
        unsigned Channel;
        unsigned Mask;

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
        unsigned Note;
        signed NoteSlide;
        signed FreqSlideHz;
        unsigned SampleNum;
        unsigned PosInSample;
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
      virtual void SetSample(unsigned idx, const Dump& data, unsigned loop) = 0;

      /// render single data chunk
      virtual void RenderData(const Sound::RenderParameters& params,
                              const DataChunk& src,
                              Sound::MultichannelReceiver& dst) = 0;

      virtual void GetState(Module::Analyze::ChannelsState& state) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(unsigned channels, unsigned samples, unsigned sampleFreq);
  }
}

#endif //__DEVICE_AYM_H_DEFINED__
