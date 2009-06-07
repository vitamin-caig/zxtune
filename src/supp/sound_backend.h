#ifndef __SOUND_BACKEND_H_DEFINED__
#define __SOUND_BACKEND_H_DEFINED__

#include <player.h>

#include "../sound/mixer.h"

namespace ZXTune
{
  namespace IO
  {
    class DataContainer;
  }

  namespace Sound
  {
    /// Asynchronous and thread-safe playing backend interface
    class Backend
    {
    public:
      typedef std::auto_ptr<Backend> Ptr;

      virtual ~Backend()
      {
      }

      //NOTOPENED <-> STOPPED -> STARTING -> STARTED <-> PAUSED
      //                      <- STOPPING <-
      enum State
      {
        NOTOPENED,
        STOPPING,
        STOPPED,
        PAUSED,
        STARTING,
        STARTED,
      };

      virtual State SetPlayer(ModulePlayer::Ptr player) = 0;

      /// Playback control
      virtual State GetState() const = 0;
      virtual State Play() = 0;
      virtual State Pause() = 0;
      virtual State Stop() = 0;

      /// Information getters
      virtual void GetPlayerInfo(ModulePlayer::Info& info) const = 0;
      virtual void GetModuleInfo(Module::Information& info) const = 0;
      virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const = 0;
      virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const = 0;

      /// Seeking
      virtual State SetPosition(const uint32_t& frame) = 0;

      /// Parameters working
      struct Parameters
      {
        Parameters()
          : DriverParameters(), DriverFlags()
          , SoundParameters(), Mixer()
          , FIROrder(), LowCutoff(), HighCutoff()
          , BufferInMs()
        {
        }
        /// Different driver parameters and flags
        String DriverParameters;
        uint32_t DriverFlags;
        /// Basic sound and/or PSG parameters
        Sound::Parameters SoundParameters;
        /// Mixing parameters
        Sound::MixerData::Ptr Mixer;
        /// FIR parameters
        std::size_t FIROrder;
        uint32_t LowCutoff;
        uint32_t HighCutoff;
        /// Rendering parameters
        std::size_t BufferInMs;

        /// Helper functions
        std::size_t BufferInMultisamples() const
        {
          return SoundParameters.SoundFreq * BufferInMs / 1000;
        }
      };
      virtual void GetSoundParameters(Parameters& params) const = 0;
      virtual void SetSoundParameters(const Parameters& params) = 0;
    };
  }
}

#endif //__SOUND_BACKEND_H_DEFINED__
