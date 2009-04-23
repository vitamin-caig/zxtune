#include "sound_backend_impl.h"

#include <tools.h>

#include <../lib/sound/mixer.h>
#include <../lib/sound/filter.h>
#include <../lib/sound/renderer.h>

#include <cassert>

namespace
{
  const unsigned PLAYTHREAD_SLEEP_PERIOD = 1000;

  using namespace ZXTune::Sound;
  void GetInitialParameters(Backend::Parameters& params)
  {
    //driver
    params.DriverParameters.clear();
    params.DriverFlags = 0;
    //sound
    params.SoundParameters.ClockFreq = 0;
    params.SoundParameters.SoundFreq = 0;
    params.SoundParameters.FrameDuration = 0;
    params.SoundParameters.Flags = 0;
    //mixing (mono by default)
    params.Mixer.clear();
    params.Preamp = FIXED_POINT_PRECISION;
    //FIR (no)
    params.FIROrder = 0;
    params.LowCutoff = params.HighCutoff = 0;
    //render (one frame)
    params.BufferInMs = 0;
  }
}

namespace ZXTune
{
  namespace Sound
  {
    BackendImpl::BackendImpl()
      : PlayerThread(), CurrentState(NOTOPENED)
    {
      GetInitialParameters(Params);
    }

    BackendImpl::~BackendImpl()
    {
      PlayerThread.Stop();
    }

    Backend::State BackendImpl::OpenModule(const String& filename, const Dump& data)
    {
      Player = ModulePlayer::Create(filename, data);
      if (Player.get())
      {
        return CurrentState = STOPPED;
      }
      CurrentState = NOTOPENED;
      throw 1;//TODO
    }

    Backend::State BackendImpl::GetState() const
    {
      return CurrentState;
    }

    Backend::State BackendImpl::Play()
    {
      CheckState();
      if (STOPPED == CurrentState)
      {
        IPC::Locker lock(PlayerMutex);
        assert(Player.get());
        Player->Reset();
        PlayerThread.Start(PlayFunc, this);
      }
      return CurrentState = PLAYING;
    }

    Backend::State BackendImpl::Pause()
    {
      CheckState();
      return CurrentState = PAUSED;
    }

    Backend::State BackendImpl::Stop()
    {
      CheckState();
      if (STOPPED != CurrentState)
      {
        CurrentState = STOPPED;
        PlayerThread.Stop();
      }
      return CurrentState;
    }

    void BackendImpl::GetPlayerInfo(ModulePlayer::Info& info) const
    {
      CheckState();
      assert(Player.get());
      IPC::Locker lock(PlayerMutex);
      return Player->GetInfo(info);
    }

    void BackendImpl::GetModuleInfo(Module::Information& info) const
    {
      CheckState();
      assert(Player.get());
      IPC::Locker lock(PlayerMutex);
      return Player->GetModuleInfo(info);
    }

    Backend::State BackendImpl::GetModuleState(uint32_t& timeState, Module::Tracking& trackState) const
    {
      CheckState();
      assert(Player.get());
      IPC::Locker lock(PlayerMutex);
      Player->GetModuleState(timeState, trackState);
      return CurrentState;
    }

    Backend::State BackendImpl::GetSoundState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const
    {
      CheckState();
      assert(Player.get());
      IPC::Locker lock(PlayerMutex);
      Player->GetSoundState(volState, spectrumState);
      return CurrentState;
    }

    /// Seeking
    Backend::State BackendImpl::SetPosition(const uint32_t& frame)
    {
      CheckState();
      if (STOPPED == CurrentState)
      {
        return CurrentState;
      }
      assert(Player.get());
      IPC::Locker lock(PlayerMutex);
      return UpdateCurrentState(Player->SetPosition(frame));
    }

    void BackendImpl::GetSoundParameters(Backend::Parameters& params) const
    {
      IPC::Locker lock(PlayerMutex);
      params = Params;
    }

    void BackendImpl::SetSoundParameters(const Backend::Parameters& params)
    {
      const unsigned UPDATE_RENDERER_MASK(BUFFER | SOUND_FRAME);
      const unsigned UPDATE_FILTER_MASK(FIR_ORDER | FIR_LOW | FIR_HIGH | SOUND_FREQ);
      const unsigned UPDATE_MIXER_MASK(MIXER);

      if (Params.Mixer.size() && params.Mixer.size() != Params.Mixer.size())
      {
        throw 2;//TODO
      }

      IPC::Locker lock(PlayerMutex);
      const unsigned changedFields(MatchParameters(params));
      Params = params;
      if (changedFields & UPDATE_RENDERER_MASK)
      {
        Renderer = CreateCallbackRenderer(Params.SoundParameters.SoundFreq * Params.BufferInMs / 1000, CallbackFunc, this);
      }
      Filter.reset();
      if ((changedFields & (UPDATE_RENDERER_MASK | UPDATE_FILTER_MASK)) && Params.FIROrder)
      {
        if (changedFields & UPDATE_FILTER_MASK)
        {
          CalculateFIRCoefficients(Params.FIROrder, Params.SoundParameters.SoundFreq, Params.LowCutoff, Params.HighCutoff, FilterCoeffs);
        }
        
        Filter = CreateFIRFilter(&FilterCoeffs[0], Params.FIROrder, Renderer.get());
      }
      if (changedFields & (UPDATE_RENDERER_MASK | UPDATE_FILTER_MASK | UPDATE_MIXER_MASK))
      {
        Mixer = CreateMixer(Params.Mixer, Filter.get() ? Filter.get() : Renderer.get());
      }
      return OnParametersChanged(changedFields);
    }

    void BackendImpl::CheckState() const
    {
      if (NOTOPENED == CurrentState)
      {
        throw 1;//TODO
      }
    }

    Backend::State BackendImpl::UpdateCurrentState(ModulePlayer::State state)
    {
      //do not change state if playing or pause if underlaying player is not stopped
      return CurrentState = ModulePlayer::MODULE_STOPPED == state ? STOPPED : CurrentState;
    }

    unsigned BackendImpl::MatchParameters(const Backend::Parameters& after) const
    {
      unsigned res(0);
      if (Params.DriverParameters != after.DriverParameters)
      {
        res |= DRIVER_PARAMS;
      }
      if (Params.DriverFlags != after.DriverFlags)
      {
        res |= DRIVER_FLAGS;
      }
      if (Params.SoundParameters.SoundFreq != after.SoundParameters.SoundFreq)
      {
        res |= SOUND_FREQ;
      }
      if (Params.SoundParameters.ClockFreq != after.SoundParameters.ClockFreq)
      {
        res |= SOUND_CLOCK;
      }
      if (Params.SoundParameters.FrameDuration != after.SoundParameters.FrameDuration)
      {
        res |= SOUND_FRAME;
      }
      if (Params.SoundParameters.Flags != after.SoundParameters.Flags)
      {
        res |= SOUND_FLAGS;
      }
      if (Params.Mixer != after.Mixer)
      {
        res |= MIXER;
      }
      if (Params.Preamp != after.Preamp)
      {
        res |= PREAMP;
      }
      if (Params.FIROrder != after.FIROrder)
      {
        res |= FIR_ORDER;
      }
      if (Params.LowCutoff != after.LowCutoff)
      {
        res |= FIR_LOW;
      }
      if (Params.HighCutoff != after.HighCutoff)
      {
        res |= FIR_HIGH;
      }
      if (Params.BufferInMs != after.BufferInMs)
      {
        res |= BUFFER;
      }
      return res;
    }

    bool BackendImpl::PlayFunc(void* obj)
    {
      BackendImpl* const self(safe_ptr_cast<BackendImpl*>(obj));
      if (PLAYING == self->CurrentState)
      {
        IPC::Locker lock(self->PlayerMutex);
        const ModulePlayer::State thatState(self->Player->RenderFrame(self->Params.SoundParameters, self->Mixer.get()));
        if (ModulePlayer::MODULE_STOPPED == thatState)
        {
          self->CurrentState = STOPPED;
        }
      }
      if (STOPPED == self->CurrentState)
      {
        self->OnShutdown();
        return false;
      }
      else if (PAUSED == self->CurrentState)
      {
        IPC::Sleep(PLAYTHREAD_SLEEP_PERIOD);
      }
      return true;
    }

    void BackendImpl::CallbackFunc(const void* data, std::size_t size, void* obj)
    {
      assert(obj);
      return safe_ptr_cast<BackendImpl*>(obj)->OnBufferReady(data, size);
    }
  }
}