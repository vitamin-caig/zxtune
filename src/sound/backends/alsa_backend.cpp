/*
Abstract:
  ALSA backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef ALSA_SUPPORT

//local includes
#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/sound_parameters.h>
//platform-specific includes
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
//boost includes
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 8B5627E4

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::ALSA");

  const Char ALSA_BACKEND_ID[] = {'a', 'l', 's', 'a', 0};
  const String ALSA_BACKEND_VERSION(FromStdString("$Rev$"));

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  inline void CheckResult(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
        Text::SOUND_ERROR_ALSA_BACKEND_ERROR, ::snd_strerror(res));
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
          Text::SOUND_ERROR_ALSA_BACKEND_DEVICE_ERROR, Name, ::snd_strerror(res));
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
        //do not break if error while drain- we need to close
        ::snd_pcm_drain(Handle);
        CheckResult(::snd_pcm_close(Handle), THIS_LINE);
        Handle = 0;
        Name.clear();
      }
    }
  };

  class AutoMixer : public AutoHandle<snd_mixer_t>
  {
  public:
    AutoMixer()
      : MixerElement(0)
    {
    }

    explicit AutoMixer(const String& deviceName, const String& mixerName)
      : AutoHandle<snd_mixer_t>(deviceName)
      , MixerName(mixerName)
      , MixerElement(0)
    {
      CheckResult(::snd_mixer_open(&Handle, 0), THIS_LINE);
      CheckResult(::snd_mixer_attach(Handle, IO::ConvertToFilename(Name).c_str()), THIS_LINE);
      CheckResult(::snd_mixer_selem_register(Handle, 0, 0), THIS_LINE);
      CheckResult(::snd_mixer_load(Handle), THIS_LINE);
      //find mixer element
      snd_mixer_elem_t* elem = ::snd_mixer_first_elem(Handle);
      snd_mixer_selem_id_t* sid = 0;
      snd_mixer_selem_id_alloca(&sid);
      while (elem)
      {
        const snd_mixer_elem_type_t type = ::snd_mixer_elem_get_type(elem);
        if (type == SND_MIXER_ELEM_SIMPLE &&
            ::snd_mixer_selem_has_playback_volume(elem) != 0)
        {
          ::snd_mixer_selem_get_id(elem, sid);
          const String mixName(FromStdString(::snd_mixer_selem_id_get_name(sid)));
          Log::Debug(THIS_MODULE, "Checking for mixer %1%", mixName);
          if (MixerName.empty())
          {
            Log::Debug(THIS_MODULE, "Using first mixer: %1%", mixName);
            MixerName = mixName;
            break;
          }
          else if (MixerName == mixName)
          {
            Log::Debug(THIS_MODULE, "Found mixer: %1%", mixName);
            break;
          }
        }
        elem = ::snd_mixer_elem_next(elem);
      }
      if (!elem)
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_ALSA_BACKEND_NO_MIXER, MixerName);
      }
      MixerElement = elem;
    }

    ~AutoMixer()
    {
      if (Handle)
      {
        ::snd_mixer_detach(Handle, IO::ConvertToFilename(Name).c_str());
        ::snd_mixer_close(Handle);
        Handle = 0;
        MixerElement = 0;
      }
    }

    void Swap(AutoMixer& rh)
    {
      std::swap(rh.Handle, Handle);
      std::swap(rh.Name, Name);
      std::swap(rh.MixerName, MixerName);
      std::swap(rh.MixerElement, MixerElement);
    }

    void Close()
    {
      if (Handle)
      {
        //do not break while detach
        ::snd_mixer_detach(Handle, IO::ConvertToFilename(Name).c_str());
        CheckResult(::snd_mixer_close(Handle), THIS_LINE);
        Handle = 0;
        MixerElement = 0;
        Name.clear();
        MixerName.clear();
      }
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      if (!Handle)
      {
        volume = MultiGain();
        return Error();
      }
      try
      {
        BOOST_STATIC_ASSERT(MultiGain::static_size == 2);
        long minVol = 0, maxVol = 0;
        CheckResult(::snd_mixer_selem_get_playback_volume_range(MixerElement, &minVol, &maxVol), THIS_LINE);
        const long volRange = maxVol - minVol;

        long leftVol = 0, rightVol = 0;
        CheckResult(::snd_mixer_selem_get_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_LEFT, &leftVol), THIS_LINE);
        CheckResult(::snd_mixer_selem_get_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_RIGHT, &rightVol), THIS_LINE);
        volume[0] = Gain(leftVol - minVol) / volRange;
        volume[1] = Gain(rightVol - minVol) / volRange;
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    virtual Error SetVolume(const MultiGain& volume)
    {
      if (volume.end() != std::find_if(volume.begin(), volume.end(), std::bind2nd(std::greater<Gain>(), Gain(1.0))))
      {
        return Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_GAIN);
      }
      if (!Handle)
      {
        return Error();
      }
      try
      {
        BOOST_STATIC_ASSERT(MultiGain::static_size == 2);
        long minVol = 0, maxVol = 0;
        CheckResult(::snd_mixer_selem_get_playback_volume_range(MixerElement, &minVol, &maxVol), THIS_LINE);
        const long volRange = maxVol - minVol;

        const long leftVol = static_cast<long>(volume[0] * volRange) + minVol;
        const long rightVol = static_cast<long>(volume[1] * volRange) + minVol;
        CheckResult(::snd_mixer_selem_set_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_LEFT, leftVol), THIS_LINE);
        CheckResult(::snd_mixer_selem_set_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_RIGHT, rightVol), THIS_LINE);

        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

  private:
    String MixerName;
    snd_mixer_elem_t* MixerElement;
  };

  class AlsaVolumeControl : public VolumeControl
  {
  public:
    AlsaVolumeControl(boost::mutex& backendMutex, AutoMixer& mixer)
      : BackendMutex(backendMutex), Mixer(mixer)
    {
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      Log::Debug(THIS_MODULE, "GetVolume");
      boost::lock_guard<boost::mutex> lock(BackendMutex);
      return Mixer.GetVolume(volume);
    }

    virtual Error SetVolume(const MultiGain& volume)
    {
      Log::Debug(THIS_MODULE, "SetVolume");
      boost::lock_guard<boost::mutex> lock(BackendMutex);
      return Mixer.SetVolume(volume);
    }
  private:
    boost::mutex& BackendMutex;
    AutoMixer& Mixer;
  };

  class AlsaBackendParameters
  {
  public:
    explicit AlsaBackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    String GetDeviceName() const
    {
      Parameters::StringType strVal = Parameters::ZXTune::Sound::Backends::ALSA::DEVICE_DEFAULT;
      Accessor.FindStringValue(Parameters::ZXTune::Sound::Backends::ALSA::DEVICE, strVal);
      return strVal;
    }

    String GetMixerName() const
    {
      Parameters::StringType strVal;
      Accessor.FindStringValue(Parameters::ZXTune::Sound::Backends::ALSA::MIXER, strVal);
      return strVal;
    }

    uint_t GetBuffersCount() const
    {
      Parameters::IntType val = Parameters::ZXTune::Sound::Backends::ALSA::BUFFERS_DEFAULT;
      if (Accessor.FindIntValue(Parameters::ZXTune::Sound::Backends::ALSA::BUFFERS, val) &&
          (!in_range<Parameters::IntType>(val, BUFFERS_MIN, BUFFERS_MAX)))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_ALSA_BACKEND_INVALID_BUFFERS, static_cast<int_t>(val), BUFFERS_MIN, BUFFERS_MAX);
      }
      return static_cast<uint_t>(val);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class AlsaBackend : public BackendImpl
                    , private boost::noncopyable
  {
  public:
    explicit AlsaBackend(Parameters::Accessor::Ptr soundParams)
      : BackendImpl(soundParams)
      , DeviceName(Parameters::ZXTune::Sound::Backends::ALSA::DEVICE_DEFAULT)
      , Buffers(Parameters::ZXTune::Sound::Backends::ALSA::BUFFERS_DEFAULT)
      , Samplerate(RenderingParameters->SoundFreq())
      , DevHandle()
      , MixHandle()
      , CanPause(0)
      , VolumeController(new AlsaVolumeControl(BackendMutex, MixHandle))
    {
    }

    virtual ~AlsaBackend()
    {
      assert(!DevHandle.Get() || !"AlsaBackend was destroyed without stopping");
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeController;
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

    virtual void OnParametersChanged(const Parameters::Accessor& updates)
    {
      const AlsaBackendParameters curParams(updates);

      //check for parameters requires restarting
      const String& newDevice = curParams.GetDeviceName();
      const String& newMixer = curParams.GetMixerName();
      const uint_t newBuffers = curParams.GetBuffersCount();
      const uint_t newFreq = RenderingParameters->SoundFreq();

      const bool deviceChanged = newDevice != DeviceName;
      const bool mixerChanged = newMixer != MixerName;
      const bool buffersChanged = newBuffers != Buffers;
      const bool freqChanged = newFreq != Samplerate;
      if (deviceChanged || mixerChanged || buffersChanged || freqChanged)
      {
        Locker lock(BackendMutex);
        const bool needStartup(DevHandle.Get() != 0);
        DoShutdown();
        DeviceName = newDevice;
        MixerName = newMixer;
        Buffers = static_cast<unsigned>(newBuffers);
        Samplerate = newFreq;

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
      Log::Debug(THIS_MODULE, "Setting frequency to %1%", Samplerate);
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_rate, hwParams, Samplerate, 0, THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting buffers count to %1%", Buffers);
      int dir = 0;
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_periods_near, hwParams, &Buffers, &dir, THIS_LINE);
      Log::Debug(THIS_MODULE, "Actually set to %1%", Buffers);

      snd_pcm_uframes_t minBufSize(Buffers * RenderingParameters->SamplesPerFrame());
      Log::Debug(THIS_MODULE, "Setting buffer size to %1% frames", minBufSize);
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_buffer_size_near, hwParams, &minBufSize, THIS_LINE);
      Log::Debug(THIS_MODULE, "Actually set %1% frames", minBufSize);

      Log::Debug(THIS_MODULE, "Applying parameters");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params, hwParams, THIS_LINE);

      CanPause = ::snd_pcm_hw_params_can_pause(hwParams) != 0;
      Log::Debug(THIS_MODULE, CanPause ? "Hardware support pause" : "Hardware doesn't support pause");
      tmpDevice.CheckedCall(&::snd_pcm_prepare, THIS_LINE);
      AutoMixer tmpMixer(DeviceName, MixerName);

      DevHandle.Swap(tmpDevice);
      MixHandle.Swap(tmpMixer);
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
    String MixerName;
    unsigned Buffers;
    unsigned Samplerate;
    AutoDevice DevHandle;
    AutoMixer MixHandle;
    bool CanPause;
    VolumeControl::Ptr VolumeController;
  };

  class AlsaBackendCreator : public BackendCreator
                           , public boost::enable_shared_from_this<AlsaBackendCreator>
  {
  public:
    virtual String Id() const
    {
      return ALSA_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::ALSA_BACKEND_DESCRIPTION;
    }

    virtual String Version() const
    {
      return ALSA_BACKEND_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;
    }

    virtual Error CreateBackend(Parameters::Accessor::Ptr params, Backend::Ptr& result) const
    {
      const BackendInformation::Ptr info = shared_from_this();
      return SafeBackendWrapper<AlsaBackend>::Create(info, params, result, THIS_LINE);
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAlsaBackend(BackendsEnumerator& enumerator)
    {
      const BackendCreator::Ptr creator(new AlsaBackendCreator());
      enumerator.RegisterCreator(creator);
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
