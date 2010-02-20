/*
Abstract:
  ALSA backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef ALSA_SUPPORT

#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"

#include <byteorder.h>
#include <error_tools.h>
#include <io/fs_tools.h>
#include <logging.h>
#include <tools.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>

//platform-specific
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>

#include <text/backends.h>
#include <text/sound.h>

#define FILE_TAG 8B5627E4

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("ALSABackend");

  const Char BACKEND_ID[] = {'a', 'l', 's', 'a', 0};
  const String BACKEND_VERSION(FromStdString("$Rev$"));

  const BackendInformation BACKEND_INFO =
  {
    BACKEND_ID,
    TEXT_ALSA_BACKEND_DESCRIPTION,
    BACKEND_VERSION,
  };

  inline void CheckResult(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
        TEXT_SOUND_ERROR_ALSA_BACKEND_ERROR, ::snd_strerror(res));
    }
  }

  template<class T>
  class AutoHandle : public boost::noncopyable
  {
  public:
    AutoHandle()
      : Handle(0)
    {
    }
    
    explicit AutoHandle(const String& name)
      : Name(name)
      , Handle(0)
    {
    }
    
    void Swap(AutoHandle<T>& rh)
    {
      std::swap(rh.Handle, Handle);
      std::swap(rh.Name, Name);
    }
    
    T* Get() const
    {
      return Handle;
    }
    
    void CheckedCall(int (*func)(T*), Error::LocationRef loc) const
    {
      CheckResult(func(Handle), loc);
    }
    
    template<class P1>
    void CheckedCall(int (*func)(T*, P1), P1 p1, Error::LocationRef loc) const
    {
      CheckResult(func(Handle, p1), loc);
    }
    
    template<class P1, class P2>
    void CheckedCall(int (*func)(T*, P1, P2), P1 p1, P2 p2, Error::LocationRef loc) const
    {
      CheckResult(func(Handle, p1, p2), loc);
    }

    template<class P1, class P2, class P3>
    void CheckedCall(int (*func)(T*, P1, P2, P3), P1 p1, P2 p2, P3 p3, Error::LocationRef loc) const
    {
      CheckResult(func(Handle, p1, p2, p3), loc);
    }

  protected:
    void CheckResult(int res, Error::LocationRef loc) const
    {
      if (res < 0)
      {
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
          TEXT_SOUND_ERROR_ALSA_BACKEND_DEVICE_ERROR, Name, ::snd_strerror(res));
      }
    }
  protected:
    String Name;
    T* Handle;
  };
  
  class AutoDevice : public AutoHandle<snd_pcm_t>
  {
  public:
    AutoDevice()
    {
    }
    
    explicit AutoDevice(const String& name)
      : AutoHandle<snd_pcm_t>(name)
    {
      CheckResult(::snd_pcm_open(&Handle, IO::ConvertToFilename(name).c_str(),
        SND_PCM_STREAM_PLAYBACK, 0), THIS_LINE);
    }

    ~AutoDevice()
    {
      if (Handle)
      {
        ::snd_pcm_hw_free(Handle);
        ::snd_pcm_close(Handle);
        Handle = 0;
      }
    }

    void Close()
    {
      if (Handle)
      {
        CheckResult(::snd_pcm_drain(Handle), THIS_LINE);
        CheckResult(::snd_pcm_close(Handle), THIS_LINE);
        Handle = 0;
        Name.clear();
      }
    }
  };

  class AlsaBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    AlsaBackend()
      : DeviceName(Parameters::ZXTune::Sound::Backends::ALSA::DEVICE_DEFAULT)
      , Buffers(Parameters::ZXTune::Sound::Backends::ALSA::BUFFERS_DEFAULT)
      , DevHandle()
      , CanPause(0)
    {
    }

    virtual ~AlsaBackend()
    {
      assert(!DevHandle.Get() || !"AlsaBackend was destroyed without stopping");
    }

    virtual void GetInformation(BackendInformation& info) const
    {
      info = BACKEND_INFO;
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      return Error();
    }
    
    virtual Error SetVolume(const MultiGain& volume)
    {
      return Error();
    }
    
    virtual void OnStartup()
    {
      Locker lock(BackendMutex);
      DoStartup();
    }

    virtual void OnShutdown()
    {
      Locker lock(BackendMutex);
      DoShutdown();
    }

    virtual void OnPause()
    {
      if (CanPause)
      {
        DevHandle.CheckedCall(&::snd_pcm_pause, 1, THIS_LINE);
      }
    }

    virtual void OnResume()
    {
      if (CanPause)
      {
        DevHandle.CheckedCall(&::snd_pcm_pause, 0, THIS_LINE);
      }
    }

    virtual void OnParametersChanged(const Parameters::Map& updates)
    {
      //check for parameters requires restarting
      const Parameters::StringType* const device = 
        Parameters::FindByName<Parameters::StringType>(updates, Parameters::ZXTune::Sound::Backends::ALSA::DEVICE);
      const Parameters::IntType* const buffers =
        Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::Backends::ALSA::BUFFERS);
      const Parameters::IntType* const freq = 
        Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::FREQUENCY);
      if (device || buffers || freq)
      {
        Locker lock(BackendMutex);
        const bool needStartup(DevHandle.Get() != 0);
        DoShutdown();
        if (device)
        {
          DeviceName = *device;
        }
        if (buffers)
        {
          //TODO: check if in range
          Buffers = static_cast<unsigned>(*buffers);
        }
        if (needStartup)
        {
          DoStartup();
        }
      }
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      Locker lock(BackendMutex);
      assert(0 != DevHandle.Get());
      const MultiSample* data = &buffer[0];
      std::size_t size = buffer.size();
      while (size)
      {
        const snd_pcm_sframes_t res = ::snd_pcm_writei(DevHandle.Get(), data, size);
        if (res < 0)
        {
          DevHandle.CheckedCall(&::snd_pcm_prepare, THIS_LINE);
          continue;
        }
        data += res;
        size -= res;
      }
    }
  private:
    void DoStartup()
    {
      assert(!DevHandle.Get());
      Log::Debug(THIS_MODULE, "Opening device '%1%'", DeviceName);
      
      AutoDevice tmpDevice(DeviceName);
      
      snd_pcm_format_t fmt(SND_PCM_FORMAT_UNKNOWN);
      switch (sizeof(Sample))
      {
      case 1:
        fmt = SND_PCM_FORMAT_S8;
        break;
      case 2:
        fmt = isLE() ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_S16_BE;
        break;
      case 4:
        fmt = isLE() ? SND_PCM_FORMAT_S32_LE : SND_PCM_FORMAT_S32_BE;
        break;
      default:
        assert(!"Invalid format");
      }

      snd_pcm_hw_params_t* hwParams = 0;
      snd_pcm_hw_params_alloca(&hwParams);
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_any, hwParams, THIS_LINE);
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_access, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED, THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting resampling possibility");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_rate_resample, hwParams, 1u, THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting format");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_format, hwParams, fmt, THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting channels");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_channels, hwParams, unsigned(OUTPUT_CHANNELS), THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting frequency");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_rate, hwParams, unsigned(RenderingParameters.SoundFreq), 0, THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting buffers count to %1%", Buffers);
      int dir = 0;
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_periods_near, hwParams, &Buffers, &dir, THIS_LINE);
      Log::Debug(THIS_MODULE, "Actually set to %1%", Buffers);

      snd_pcm_uframes_t minBufSize(Buffers * RenderingParameters.SamplesPerFrame());
      Log::Debug(THIS_MODULE, "Setting buffer size to %1% frames", minBufSize);
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_buffer_size_near, hwParams, &minBufSize, THIS_LINE);
      Log::Debug(THIS_MODULE, "Actually set %1% frames", minBufSize);

      Log::Debug(THIS_MODULE, "Applying parameters");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params, hwParams, THIS_LINE);

      CanPause = ::snd_pcm_hw_params_can_pause(hwParams) != 0;
      Log::Debug(THIS_MODULE, CanPause ? "Hardware support pause" : "Hardware doesn't support pause");
      tmpDevice.CheckedCall(&::snd_pcm_prepare, THIS_LINE);
        
      DevHandle.Swap(tmpDevice);
      Log::Debug(THIS_MODULE, "Successfully opened");
    }

    void DoShutdown()
    {
      Log::Debug(THIS_MODULE, "Closing all the devices");
      DevHandle.Close();
      CheckResult(snd_config_update_free_global(), THIS_LINE);
      Log::Debug(THIS_MODULE, "Successfully closed");
    }

  private:
    String DeviceName;
    unsigned Buffers;
    AutoDevice DevHandle;
    bool CanPause;
  };

  Backend::Ptr AlsaBackendCreator(const Parameters::Map& params)
  {
    return Backend::Ptr(new SafeBackendWrapper<AlsaBackend>(params));
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAlsaBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterBackend(BACKEND_INFO, AlsaBackendCreator, BACKEND_PRIORITY_HIGH);
    }
  }
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAlsaBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}

#endif
