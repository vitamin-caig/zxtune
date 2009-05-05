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
    params.SoundParameters.ClockFreq = 1750000;
    params.SoundParameters.SoundFreq = 44100;
    params.SoundParameters.FrameDuration = 20;
    params.SoundParameters.Flags = 0;
    //mixing (no channels)
    params.Preamp = FIXED_POINT_PRECISION;
    //FIR (no)
    params.FIROrder = 0;
    params.LowCutoff = params.HighCutoff = 0;
    //render (one frame)
    params.BufferInMs = 500;
  }
}

namespace ZXTune
{
  namespace Sound
  {
    BackendImpl::BackendImpl()
      : Params()
      , PlayerThread(), PlayerMutex()
      , CurrentState(NOTOPENED)
      , Player(), Mixer(), Filter(), FilterCoeffs(), Renderer()
    {
      GetInitialParameters(Params);
    }

    Backend::State BackendImpl::OpenModule(const String& filename, const Dump& data)
    {
      static const SampleArray MONO_MIX = {FIXED_POINT_PRECISION};

      IPC::Locker locker(PlayerMutex);
      SafeStop();
      Player.reset(ModulePlayer::Create(filename, data).release());
      if (Player.get())
      {
        Module::Information info;
        Player->GetModuleInfo(info);
        Params.Mixer.resize(info.Statistic.Channels, ChannelMixer(MONO_MIX));
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
      IPC::Locker lock(PlayerMutex);
      CheckState();
      if (STOPPED == CurrentState)
      {
        assert(Player.get());
        Player->Reset();
        OnStartup();
        PlayerThread.Start(PlayFunc, this);
      }
      else if (PAUSED == CurrentState)
      {
        OnResume();
      }
      return CurrentState = PLAYING;
    }

    Backend::State BackendImpl::Pause()
    {
      IPC::Locker lock(PlayerMutex);
      CheckState();
      OnPause();
      return CurrentState = PAUSED;
    }

    Backend::State BackendImpl::Stop()
    {
      //do not lock anything
      CheckState();
      SafeStop();
      return CurrentState;
    }

    void BackendImpl::GetPlayerInfo(ModulePlayer::Info& info) const
    {
      IPC::Locker lock(PlayerMutex);
      CheckState();
      assert(Player.get());
      return Player->GetInfo(info);
    }

    void BackendImpl::GetModuleInfo(Module::Information& info) const
    {
      IPC::Locker lock(PlayerMutex);
      CheckState();
      assert(Player.get());
      return Player->GetModuleInfo(info);
    }

    Backend::State BackendImpl::GetModuleState(uint32_t& timeState, Module::Tracking& trackState) const
    {
      IPC::Locker lock(PlayerMutex);
      CheckState();
      assert(Player.get());
      Player->GetModuleState(timeState, trackState);
      return CurrentState;
    }

    Backend::State BackendImpl::GetSoundState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const
    {
      IPC::Locker lock(PlayerMutex);
      CheckState();
      assert(Player.get());
      Player->GetSoundState(volState, spectrumState);
      return CurrentState;
    }

    /// Seeking
    Backend::State BackendImpl::SetPosition(const uint32_t& frame)
    {
      IPC::Locker lock(PlayerMutex);
      CheckState();
      if (STOPPED == CurrentState)
      {
        return CurrentState;
      }
      assert(Player.get());
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
        Renderer.reset(
          CreateCallbackRenderer(Params.SoundParameters.SoundFreq * Params.BufferInMs / 1000, CallbackFunc, this).release());
      }
      Filter.reset();
      if ((changedFields & (UPDATE_RENDERER_MASK | UPDATE_FILTER_MASK)) && Params.FIROrder)
      {
        if (changedFields & UPDATE_FILTER_MASK)
        {
          CalculateFIRCoefficients(Params.FIROrder, Params.SoundParameters.SoundFreq, Params.LowCutoff, Params.HighCutoff, FilterCoeffs);
        }

        Filter.reset(
          CreateFIRFilter(&FilterCoeffs[0], Params.FIROrder, Renderer.get()).release());
      }
      if (changedFields & (UPDATE_RENDERER_MASK | UPDATE_FILTER_MASK | UPDATE_MIXER_MASK))
      {
        Mixer.reset(
          CreateMixer(Params.Mixer, Filter.get() ? Filter.get() : Renderer.get()).release());
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

    void BackendImpl::SafeStop()
    {
      if (PLAYING == CurrentState || PAUSED == CurrentState)
      {
        CurrentState = STOPPED;
        PlayerThread.Stop();
        OnShutdown();
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
      {
        IPC::Locker lock(self->PlayerMutex);
        if (PLAYING == self->CurrentState)
        {
          const ModulePlayer::State thatState(self->Player->RenderFrame(self->Params.SoundParameters, self->Mixer.get()));
          if (ModulePlayer::MODULE_STOPPED == thatState)
          {
            self->CurrentState = STOPPED;
            self->OnShutdown();
          }
        }
        if (STOPPED == self->CurrentState)
        {
          return false;
        }
      }
      if (PAUSED == self->CurrentState)
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
