#ifndef __DAC_H_DEFINED__
#define __DAC_H_DEFINED__

#include <types.h>
#include <sound.h>

#include "../data_source.h"

//supporting for multichannel sample-based DAC
namespace ZXTune
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
        std::size_t Channel;
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
        std::size_t Note;
        signed NoteSlide;
        signed FreqSlideHz;
        std::size_t SampleNum;
        std::size_t PosInSample;
      };

      DataChunk() : Tick()
      {
      }
      uint64_t Tick;
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
      virtual void SetSample(std::size_t idx, const Dump& data, std::size_t loop) = 0;

      /// render single data chunk
      virtual void RenderData(const Sound::Parameters& params,
        const DataChunk& src,
        Sound::Receiver& dst) = 0;

      virtual void GetState(Sound::Analyze::ChannelsState& state) const = 0;

      /// reset internal state to initial
      virtual void Reset() = 0;
    };

    /// Virtual constructors
    Chip::Ptr CreateChip(std::size_t channels, std::size_t samples, std::size_t sampleFreq);
  }
}

#endif //__DEVICE_AYM_H_DEFINED__
