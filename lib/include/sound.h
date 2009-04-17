#ifndef __SOUND_H_DEFINED__
#define __SOUND_H_DEFINED__

#include <types.h>
#include <sound_types.h>

namespace ZXTune
{
  namespace Sound
  {
    /// Sound chunks mixer and consumer
    class Receiver
    {
    public:
      typedef std::auto_ptr<Receiver> Ptr;

      virtual ~Receiver()
      {
      }

      /// Performs mixing, processing and storing
      virtual void ApplySample(const Sample* input, std::size_t channels) = 0;
    };

    /// Input parameters for rendering
    struct Parameters
    {
      /// Rendering sound frequency
      uint32_t SoundFreq;
      /// Basic clock frequency (for PSG,CPU etc)
      uint32_t ClockFreq;
      /// Frame duration in ms
      uint32_t FrameDuration;
      /// Different flags
      uint32_t Flags;
    };

    namespace Analyze
    {
      /// Level type (by default 0...255 is enough)
      typedef uint8_t Level;

      /// Volume analyze result
      struct Volume
      {
        /// Now active channels masks
        unsigned ChannelsMask;
        /// Volumes for ALL channels
        std::vector<Level> Array;
      };

      /// Spectrum analyzing constants
      const std::size_t OctavesCount = 8;
      const std::size_t BandsCount = 12;
      const std::size_t TonesCount = OctavesCount * BandsCount;
      /// Spectrum analyze result
      struct Spectrum
      {
        Level Array[TonesCount];
      };
    }
  }
}

#endif //__SOUND_H_DEFINED__
