#ifndef __SOUND_H_DEFINED__
#define __SOUND_H_DEFINED__

#include <types.h>
#include <sound_types.h>

#include <boost/array.hpp>

#include <memory>

namespace ZXTune
{
  namespace Sound
  {
    /// Sound chunks mixer and consumer
    class Receiver
    {
    public:
      typedef boost::shared_ptr<Receiver> Ptr;

      virtual ~Receiver()
      {
      }

      /// Performs mixing, processing and storing
      virtual void ApplySample(const Sample* input, std::size_t channels) = 0;

      /// Flush collected data
      virtual void Flush() = 0;
    };

    /// Receiver interface extention for special cases
    class Convertor : public Receiver
    {
    public:
      typedef boost::shared_ptr<Convertor> Ptr;
      //change endpoint
      virtual void SetEndpoint(Receiver::Ptr rcv) = 0;
    };

    /// Input parameters for rendering
    struct Parameters
    {
      /// Rendering sound frequency
      uint32_t SoundFreq;
      /// Basic clock frequency (for PSG,CPU etc)
      uint32_t ClockFreq;
      /// Frame duration in us
      uint32_t FrameDurationMicrosec;
      /// Different flags
      uint32_t Flags;

      /// Helper functions
      uint32_t ClocksPerFrame() const
      {
        return uint32_t(uint64_t(ClockFreq) * FrameDurationMicrosec / 1e6);
      }

      uint32_t SamplesPerFrame() const
      {
        return uint32_t(uint64_t(SoundFreq) * FrameDurationMicrosec / 1e6);
      }
    };

    namespace Analyze
    {
      /// Level type (by default 0...255 is enough)
      typedef uint8_t LevelType;

      /// Channel voice characteristics
      struct Channel
      {
        Channel() : Enabled(), Level(), Band()
        {
        }
        bool Enabled;
        LevelType Level;
        std::size_t Band;
      };

      typedef std::vector<Channel> ChannelsState;
    }
  }
}

#endif //__SOUND_H_DEFINED__
