#ifndef __SOUND_BACKEND_IMPL_H_DEFINED__
#define __SOUND_BACKEND_IMPL_H_DEFINED__

#include "sound_backend.h"

#include <boost/thread.hpp>

namespace ZXTune
{
  namespace Sound
  {
    class BackendImpl : public Backend
    {
    public:
      BackendImpl();
      virtual ~BackendImpl();

      virtual State SetPlayer(ModulePlayer::Ptr player);

      virtual State GetState() const;
      virtual State Play();
      virtual State Pause();
      virtual State Stop();

      /// Information getters
      virtual void GetPlayerInfo(ModulePlayer::Info& info) const;
      virtual void GetModuleInfo(Module::Information& info) const;
      virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const;
      virtual State GetSoundState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const;

      /// Seeking
      virtual State SetPosition(const uint32_t& frame);
      virtual void GetSoundParameters(Parameters& params) const;
      virtual void SetSoundParameters(const Parameters& params);

    protected:
      virtual void OnBufferReady(const void* data, std::size_t sizeInBytes) = 0;
      //changed fields
      enum
      {
        DRIVER_PARAMS = 1,
        DRIVER_FLAGS = 2,
        SOUND_FREQ = 4,
        SOUND_CLOCK = 8,
        SOUND_FRAME = 16,
        SOUND_FLAGS = 32,
        MIXER = 64,
        PREAMP = 128,
        FIR_ORDER = 256,
        FIR_LOW = 512,
        FIR_HIGH = 1024,
        BUFFER = 2048
      };
      virtual void OnParametersChanged(unsigned changedFields) = 0;
      virtual void OnStartup() = 0;
      virtual void OnShutdown() = 0;
      virtual void OnPause() = 0;
      virtual void OnResume() = 0;
    protected:
      Parameters Params;

    private:
      ModulePlayer::State SafeRenderFrame();
      void PlayFunc();
      void CheckState() const;
      void SafeStop();
      State UpdateCurrentState(ModulePlayer::State);
      unsigned MatchParameters(const Parameters& params) const;
      static void CallbackFunc(const void*, std::size_t, void*);
    private:
      boost::thread PlayerThread;
      mutable boost::mutex PlayerMutex;
    private:
      volatile State CurrentState;
      ModulePlayer::Ptr Player;
      Convertor::Ptr Mixer;
      Convertor::Ptr Filter;
      std::vector<Sample> FilterCoeffs;
      Receiver::Ptr Renderer;
    };
  }
}


#endif //__SOUND_BACKEND_IMPL_H_DEFINED__
