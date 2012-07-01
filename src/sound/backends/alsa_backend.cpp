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
#include "alsa.h"
#include "backend_impl.h"
#include "enumerator.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <shared_library_gate.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/thread/mutex.hpp>
//platform-specific includes
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
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

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  class AlsaName : public SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "asound";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libasound.so.2.0.0",//deb-based
        "libasound.so.2",    //rpm-based
      };
      return std::vector<std::string>(ALTERNATIVES, ArrayEnd(ALTERNATIVES));
    }
    
    virtual std::vector<std::string> WindowsAlternatives() const
    {
      return std::vector<std::string>();
    }
  } Names;

  struct AlsaLibraryTraits
  {
    static const SharedLibrary::Name& GetName()
    {
      return Names;
    }

    static void Startup()
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    static void Shutdown()
    {
      //Due to static instances of traits, this call may cause crash
      //::snd_config_update_free_global();
      Log::Debug(THIS_MODULE, "Library unloaded");
    }
  };
  
  typedef SharedLibraryGate<AlsaLibraryTraits> AlsaLibrary;
  
  inline void CheckResult(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
        Text::SOUND_ERROR_ALSA_BACKEND_ERROR, ::snd_strerror(res));
    }
  }

  class SoundFormat
  {
  public:
    explicit SoundFormat(snd_pcm_format_mask_t* mask)
      : Native(GetSoundFormat(SAMPLE_SIGNED))
      , Negated(GetSoundFormat(!SAMPLE_SIGNED))
      , NativeSupported(::snd_pcm_format_mask_test(mask, Native))
      , NegatedSupported(::snd_pcm_format_mask_test(mask, Negated))
    {
    }

    bool IsSupported() const
    {
      return NativeSupported || NegatedSupported;
    }

    snd_pcm_format_t Get() const
    {
      return NativeSupported
        ? Native
        : (NegatedSupported ? Negated : SND_PCM_FORMAT_UNKNOWN);
    }

    bool ChangeSign() const
    {
      return !NativeSupported && NegatedSupported;
    }
  private:
    static snd_pcm_format_t GetSoundFormat(bool isSigned)
    {
      switch (sizeof(Sample))
      {
      case 1:
        return isSigned ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_U8;
      case 2:
        return isSigned
            ? (isLE() ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_S16_BE)
            : (isLE() ? SND_PCM_FORMAT_U16_LE : SND_PCM_FORMAT_U16_BE);
      default:
        assert(!"Invalid format");
        return SND_PCM_FORMAT_UNKNOWN;
      }
    }
  private:
    const snd_pcm_format_t Native;
    const snd_pcm_format_t Negated;
    const bool NativeSupported;
    const bool NegatedSupported;
  };

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

    T* Release()
    {
      T* tmp = 0;
      std::swap(tmp, Handle);
      Name.clear();
      return tmp;
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

  class AlsaIdentifier
  {
  public:
    explicit AlsaIdentifier(const String& id)
    {
      static const Char DELIMITERS[] = {':', ','};
      StringArray elements;
      boost::algorithm::split(elements, id, boost::algorithm::is_any_of(DELIMITERS));
      elements.resize(3);
      Interface = elements[0];
      Card = elements[1];
      Device = elements[2];
    }

    String GetCard() const
    {
      String res = Interface;
      if (!Card.empty())
      {
        res += ':';
        res += Card;
      }
      return res;
    }

    String GetPCM() const
    {
      String res = Interface;
      if (!Card.empty())
      {
        res += ':';
        res += Card;
        if (!Device.empty())
        {
          res += ',';
          res += Device;
        }
      }
      return res;
    }
  private:
    String Interface;
    String Card;
    String Device;
  };

  class AutoDevice : public AutoHandle<snd_pcm_t>
  {
  public:
    AutoDevice()
    {
    }

    explicit AutoDevice(const AlsaIdentifier& id)
      : AutoHandle<snd_pcm_t>(id.GetPCM())
    {
      Log::Debug(THIS_MODULE, "Opening device '%1%'", Name);
      CheckResult(::snd_pcm_open(&Handle, IO::ConvertToFilename(Name).c_str(),
        SND_PCM_STREAM_PLAYBACK, 0), THIS_LINE);
    }

    ~AutoDevice()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {
      }
    }

    void Close()
    {
      if (Handle)
      {
        //do not break if error while drain- we need to close
        ::snd_pcm_drain(Handle);
        Log::Debug(THIS_MODULE, "Closing device '%1%'", Name);
        ::snd_pcm_hw_free(Handle);
        CheckResult(::snd_pcm_close(Release()), THIS_LINE);
      }
    }
  };

  class MixerElementsIterator
  {
  public:
    explicit MixerElementsIterator(snd_mixer_t& mixer)
      : Current(::snd_mixer_first_elem(&mixer))
    {
      SkipNotsupported();
    }

    bool IsValid() const
    {
      return Current != 0;
    }

    snd_mixer_elem_t* Get() const
    {
      return Current;
    }

    void Next()
    {
      Current = ::snd_mixer_elem_next(Current);
      SkipNotsupported();
    }
  private:
    void SkipNotsupported()
    {
      while (Current)
      {
        const snd_mixer_elem_type_t type = ::snd_mixer_elem_get_type(Current);
        if (type == SND_MIXER_ELEM_SIMPLE &&
            ::snd_mixer_selem_has_playback_volume(Current) != 0)
        {
          break;
        }
        Current = ::snd_mixer_elem_next(Current);
      }
    }
  private:
    snd_mixer_elem_t* Current;
  };

  class AutoMixer : public AutoHandle<snd_mixer_t>
  {
  public:
    AutoMixer()
      : MixerElement(0)
    {
    }

    explicit AutoMixer(const AlsaIdentifier& id)
      : AutoHandle<snd_mixer_t>(id.GetCard())
      , MixerName()
      , MixerElement(0)
    {
      Log::Debug(THIS_MODULE, "Opening mixer '%1%'", Name);
      CheckResult(::snd_mixer_open(&Handle, 0), THIS_LINE);
      CheckedCall(&::snd_mixer_attach, IO::ConvertToFilename(Name).c_str(), THIS_LINE);
      CheckedCall(&::snd_mixer_selem_register, static_cast<snd_mixer_selem_regopt*>(0), static_cast<snd_mixer_class_t**>(0), THIS_LINE);
      CheckedCall(&::snd_mixer_load, THIS_LINE);
      Select(String());
    }

    ~AutoMixer()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {
      }
    }

    void Swap(AutoMixer& rh)
    {
      AutoHandle<snd_mixer_t>::Swap(rh);
      std::swap(rh.MixerName, MixerName);
      std::swap(rh.MixerElement, MixerElement);
    }

    void Close()
    {
      if (Handle)
      {
        Log::Debug(THIS_MODULE, "Closing mixer for device '%1%'", Name);
        //do not break while detach
        ::snd_mixer_detach(Handle, IO::ConvertToFilename(Name).c_str());
        MixerElement = 0;
        MixerName.clear();
        CheckResult(::snd_mixer_close(Release()), THIS_LINE);
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

    void Select(const String& name)
    {
      Log::Debug(THIS_MODULE, "Opening mixer '%1% for device '%2%'", name, Name);
      //find mixer element
      for (MixerElementsIterator iter(*Handle); iter.IsValid(); iter.Next())
      {
        snd_mixer_elem_t* const elem = iter.Get();
        const String mixName(FromStdString(::snd_mixer_selem_get_name(elem)));
        Log::Debug(THIS_MODULE, "Checking for mixer %1%", mixName);
        if (name.empty())
        {
          Log::Debug(THIS_MODULE, "Using first mixer: %1%", mixName);
          MixerName = mixName;
          MixerElement = elem;
          break;
        }
        else if (name == mixName)
        {
          Log::Debug(THIS_MODULE, "Found mixer: %1%", mixName);
          MixerName = mixName;
          MixerElement = elem;
          break;
        }
      }
      if (!MixerElement)
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_ALSA_BACKEND_NO_MIXER, MixerName);
      }
    }

    StringArray Enumerate() const
    {
      StringArray result;
      for (MixerElementsIterator iter(*Handle); iter.IsValid(); iter.Next())
      {
        snd_mixer_elem_t* const elem = iter.Get();
        const String mixName(FromStdString(::snd_mixer_selem_get_name(elem)));
        result.push_back(mixName);
      }
      return result;
    }
  private:
    String MixerName;
    snd_mixer_elem_t* MixerElement;
  };

  class AlsaVolumeControl : public VolumeControl
  {
  public:
    AlsaVolumeControl(boost::mutex& stateMutex, AutoMixer& mixer)
      : StateMutex(stateMutex), Mixer(mixer)
    {
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      Log::Debug(THIS_MODULE, "GetVolume");
      boost::mutex::scoped_lock lock(StateMutex);
      return Mixer.GetVolume(volume);
    }

    virtual Error SetVolume(const MultiGain& volume)
    {
      Log::Debug(THIS_MODULE, "SetVolume");
      boost::mutex::scoped_lock lock(StateMutex);
      return Mixer.SetVolume(volume);
    }
  private:
    boost::mutex& StateMutex;
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
      Accessor.FindValue(Parameters::ZXTune::Sound::Backends::ALSA::DEVICE, strVal);
      return strVal;
    }

    String GetMixerName() const
    {
      Parameters::StringType strVal;
      Accessor.FindValue(Parameters::ZXTune::Sound::Backends::ALSA::MIXER, strVal);
      return strVal;
    }

    uint_t GetBuffersCount() const
    {
      Parameters::IntType val = Parameters::ZXTune::Sound::Backends::ALSA::BUFFERS_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Sound::Backends::ALSA::BUFFERS, val) &&
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

  class AlsaBackendWorker : public BackendWorker
                          , private boost::noncopyable
  {
  public:
    explicit AlsaBackendWorker(Parameters::Accessor::Ptr params)
      : BackendParams(params)
      , RenderingParameters(RenderParameters::Create(BackendParams))
      , DevHandle()
      , MixHandle()
      , CanPause(0)
      , SamplesShouldBeConverted(0)
      , VolumeController(new AlsaVolumeControl(StateMutex, MixHandle))
    {
    }

    virtual ~AlsaBackendWorker()
    {
      assert(!DevHandle.Get() || !"AlsaBackend was destroyed without stopping");
    }

    virtual void Test()
    {
      AutoDevice device;
      AutoMixer mixer;
      bool canPause = false, changeSign = false;
      OpenDevices(device, mixer, canPause, changeSign);
      Log::Debug(THIS_MODULE, "Checked!");
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeController;
    }

    virtual void OnStartup(const Module::Holder& /*module*/)
    {
      assert(!DevHandle.Get());
      OpenDevices(DevHandle, MixHandle, CanPause, SamplesShouldBeConverted);
      Log::Debug(THIS_MODULE, "Successfully opened");
    }

    virtual void OnShutdown()
    {
      DevHandle.Close();
      //Do not close mixer
      //this call causes crash if working with libasound.so directly
      //::snd_config_update_free_global();
      Log::Debug(THIS_MODULE, "Successfully closed");
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

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }

    virtual void OnBufferReady(Chunk& buffer)
    {
      if (SamplesShouldBeConverted)
      {
        std::transform(buffer.front().begin(), buffer.back().end(), buffer.front().begin(), &ToSignedSample);
      }
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
    void OpenDevices(AutoDevice& device, AutoMixer& mixer, bool& canPause, bool& changeSign) const
    {
      const AlsaBackendParameters params(*BackendParams);

      const AlsaIdentifier deviceId(params.GetDeviceName());
      AutoDevice tmpDevice(deviceId);

      snd_pcm_hw_params_t* hwParams = 0;
      snd_pcm_hw_params_alloca(&hwParams);

      tmpDevice.CheckedCall(&::snd_pcm_hw_params_any, hwParams, THIS_LINE);
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_access, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED, THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting resampling possibility");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_rate_resample, hwParams, 1u, THIS_LINE);

      snd_pcm_format_mask_t* fmtMask = 0;
      snd_pcm_format_mask_alloca(&fmtMask);
      ::snd_pcm_hw_params_get_format_mask(hwParams, fmtMask);
      const SoundFormat fmt(fmtMask);
      Log::Debug(THIS_MODULE, "Setting format");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_format, hwParams, fmt.Get(), THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting channels");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_channels, hwParams, unsigned(OUTPUT_CHANNELS), THIS_LINE);
      const unsigned samplerate = RenderingParameters->SoundFreq();
      Log::Debug(THIS_MODULE, "Setting frequency to %1%", samplerate);
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_rate, hwParams, samplerate, 0, THIS_LINE);
      unsigned buffersCount = params.GetBuffersCount();
      Log::Debug(THIS_MODULE, "Setting buffers count to %1%", buffersCount);
      int dir = 0;
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_periods_near, hwParams, &buffersCount, &dir, THIS_LINE);
      Log::Debug(THIS_MODULE, "Actually set to %1%", buffersCount);

      snd_pcm_uframes_t minBufSize(buffersCount * RenderingParameters->SamplesPerFrame());
      Log::Debug(THIS_MODULE, "Setting buffer size to %1% frames", minBufSize);
      tmpDevice.CheckedCall(&::snd_pcm_hw_params_set_buffer_size_near, hwParams, &minBufSize, THIS_LINE);
      Log::Debug(THIS_MODULE, "Actually set %1% frames", minBufSize);

      Log::Debug(THIS_MODULE, "Applying parameters");
      tmpDevice.CheckedCall(&::snd_pcm_hw_params, hwParams, THIS_LINE);

      canPause = ::snd_pcm_hw_params_can_pause(hwParams) != 0;
      changeSign = fmt.ChangeSign();
      Log::Debug(THIS_MODULE, canPause ? "Hardware support pause" : "Hardware doesn't support pause");
      tmpDevice.CheckedCall(&::snd_pcm_prepare, THIS_LINE);
      const String mixerName = params.GetMixerName();
      AutoMixer tmpMixer(deviceId);
      tmpMixer.Select(mixerName);

      device.Swap(tmpDevice);
      mixer.Swap(tmpMixer);
    }
  private:
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    boost::mutex StateMutex;
    AutoDevice DevHandle;
    AutoMixer MixHandle;
    bool CanPause;
    bool SamplesShouldBeConverted;
    const VolumeControl::Ptr VolumeController;
  };

  class AlsaBackendCreator : public BackendCreator
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

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const BackendWorker::Ptr worker(new AlsaBackendWorker(allParams));
        result = Sound::CreateBackend(params, worker);
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_FAILED_CREATE,
          Text::SOUND_ERROR_BACKEND_FAILED, Id()).AddSuberror(e);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
      }
    }
  };

  boost::shared_ptr<snd_ctl_t> OpenDevice(const std::string& deviceName)
  {
    snd_ctl_t* ctl = 0;
    return ::snd_ctl_open(&ctl, deviceName.c_str(), 0) >= 0
      ? boost::shared_ptr<snd_ctl_t>(ctl, &::snd_ctl_close)
      : boost::shared_ptr<snd_ctl_t>();
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAlsaBackend(BackendsEnumerator& enumerator)
    {
      if (AlsaLibrary::Instance().IsAccessible())
      {
        Log::Debug(THIS_MODULE, "Detected ALSA %1%", ::snd_asoundlib_version());
        const BackendCreator::Ptr creator(new AlsaBackendCreator());
        enumerator.RegisterCreator(creator);
      }
      else
      {
        Log::Debug(THIS_MODULE, "%1%", Error::ToString(AlsaLibrary::Instance().GetLoadError()));
      }
    }

    namespace ALSA
    {
      StringArray EnumerateDevices()
      {
        StringArray result;
        snd_ctl_card_info_t* cardInfo;
        snd_pcm_info_t* pcmInfo;
        snd_ctl_card_info_alloca(&cardInfo);
        snd_pcm_info_alloca(&pcmInfo);
        for (int cardIdx = -1; ::snd_card_next(&cardIdx) >= 0 && cardIdx >= 0; )
        {
          const std::string hwId = (boost::format("hw:%i") % cardIdx).str();
          const boost::shared_ptr<snd_ctl_t> card = OpenDevice(hwId);
          if (!card)
          {
            continue;
          }
          if (::snd_ctl_card_info(card.get(), cardInfo) < 0)
          {
            continue;
          }
          const std::string cardId(::snd_ctl_card_info_get_id(cardInfo));
          //const std::string cardName(::snd_ctl_card_info_get_name(cardInfo));
          for (int devIdx = -1; ::snd_ctl_pcm_next_device(card.get(), &devIdx) >= 0 && devIdx >= 0; )
          {
            ::snd_pcm_info_set_device(pcmInfo, devIdx);
            ::snd_pcm_info_set_subdevice(pcmInfo, 0);
            ::snd_pcm_info_set_stream(pcmInfo, SND_PCM_STREAM_PLAYBACK);
            if (::snd_ctl_pcm_info(card.get(), pcmInfo) < 0)
            {
              continue;
            }
            //const std::string deviceName(::snd_pcm_info_get_name(pcmInfo));
            const std::string deviceId = (boost::format("hw:%s,%u") % cardId % devIdx).str();
            result.push_back(FromStdString(deviceId));
          }
        }
        return result;
      }

      StringArray EnumerateMixers(const String& device)
      {
        try
        {
          const AlsaIdentifier id(device);
          const AutoMixer mixer(id);
          return mixer.Enumerate();
        }
        catch (const Error&)
        {
          return StringArray();
        }
      }
    }
  }
}

//global namespace
#define STR(a) #a
#define ASOUND_FUNC(func) AlsaLibrary::Instance().GetSymbol(&func, STR(func))

#include "stubs/alsa.h"

#else //not supported

//local includes
#include "alsa.h"

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAlsaBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }

    namespace ALSA
    {
      StringArray EnumerateDevices()
      {
        return StringArray();
      }

      StringArray EnumerateMixers(const String& /*device*/)
      {
        return StringArray();
      }
    }
  }
}

#endif
