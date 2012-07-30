/*
Abstract:
  ALSA backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "alsa.h"
#include "alsa_api.h"
#include "backend_impl.h"
#include "enumerator.h"
#include "volume_control.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <error_tools.h>
#include <logging.h>
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
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
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

  inline void CheckResult(Alsa::Api& api, int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
        Text::SOUND_ERROR_ALSA_BACKEND_ERROR, api.snd_strerror(res));
    }
  }

  class SoundFormat
  {
  public:
    SoundFormat(Alsa::Api& api, snd_pcm_format_mask_t* mask)
      : Native(GetSoundFormat(SAMPLE_SIGNED))
      , Negated(GetSoundFormat(!SAMPLE_SIGNED))
      , NativeSupported(api.snd_pcm_format_mask_test(mask, Native))
      , NegatedSupported(api.snd_pcm_format_mask_test(mask, Negated))
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
    AutoHandle(Alsa::Api::Ptr api, const String& name)
      : Api(api)
      , Name(name)
      , Handle(0)
    {
    }

    T* Get() const
    {
      return Handle;
    }

    void CheckedCall(int (Alsa::Api::*func)(T*), Error::LocationRef loc) const
    {
      CheckResult(((*Api).*func)(Handle), loc);
    }

    template<class P1>
    void CheckedCall(int (Alsa::Api::*func)(T*, P1), P1 p1, Error::LocationRef loc) const
    {
      CheckResult(((*Api).*func)(Handle, p1), loc);
    }

    template<class P1, class P2>
    void CheckedCall(int (Alsa::Api::*func)(T*, P1, P2), P1 p1, P2 p2, Error::LocationRef loc) const
    {
      CheckResult(((*Api).*func)(Handle, p1, p2), loc);
    }

    template<class P1, class P2, class P3>
    void CheckedCall(int (Alsa::Api::*func)(T*, P1, P2, P3), P1 p1, P2 p2, P3 p3, Error::LocationRef loc) const
    {
      CheckResult(((*Api).*func)(Handle, p1, p2, p3), loc);
    }

  protected:
    T* Release()
    {
      T* tmp = 0;
      std::swap(tmp, Handle);
      Name.clear();
      return tmp;
    }

    void CheckResult(int res, Error::LocationRef loc) const
    {
      if (res < 0)
      {
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
          Text::SOUND_ERROR_ALSA_BACKEND_DEVICE_ERROR, Name, Api->snd_strerror(res));
      }
    }
  protected:
    const Alsa::Api::Ptr Api;
    String Name;
    T* Handle;
  };

  class AlsaIdentifier
  {
  public:
    explicit AlsaIdentifier(const String& id)
    {
      static const Char DELIMITERS[] = {':', ',', 0};
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

  //Use the most atomic operations to prevent resources leak in case of error
  class PCMDevice : public AutoHandle<snd_pcm_t>
  {
  public:
    PCMDevice(Alsa::Api::Ptr api, const AlsaIdentifier& id)
      : AutoHandle<snd_pcm_t>(api, id.GetPCM())
      , Api(api)
    {
      Open();
    }
    
    ~PCMDevice()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {
      }
    }

    void Open()
    {
      Require(Handle == 0);
      Log::Debug(THIS_MODULE, "Opening PCM device '%1%'", Name);
      CheckResult(Api->snd_pcm_open(&Handle, IO::ConvertToFilename(Name).c_str(),
        SND_PCM_STREAM_PLAYBACK, 0), THIS_LINE);
    }
    
    void Close()
    {
      if (Handle)
      {
        //do not break if error while drain- we need to close
        Api->snd_pcm_drain(Handle);
        Api->snd_pcm_hw_free(Handle);
        Log::Debug(THIS_MODULE, "Closing PCM device '%1%'", Name);
        CheckResult(Api->snd_pcm_close(Release()), THIS_LINE);
      }
    }

    void Write(Chunk& buffer)
    {
      const MultiSample* data = &buffer[0];
      std::size_t size = buffer.size();
      while (size)
      {
        const snd_pcm_sframes_t res = Api->snd_pcm_writei(Handle, data, size);
        if (res < 0)
        {
          CheckedCall(&Alsa::Api::snd_pcm_prepare, THIS_LINE);
          continue;
        }
        data += res;
        size -= res;
      }
    }
  private:
    const Alsa::Api::Ptr Api;
  };
  
  template<class T>
  boost::shared_ptr<T> Allocate(Alsa::Api::Ptr api, int (Alsa::Api::*allocFunc)(T**), void (Alsa::Api::*freeFunc)(T*))
  {
    T* res = 0;
    CheckResult(*api, ((*api).*allocFunc)(&res), THIS_LINE);
    return res
      ? boost::shared_ptr<T>(res, boost::bind(freeFunc, api, _1))
      : boost::shared_ptr<T>();
  }

  class Device
  {
  public:
    typedef boost::shared_ptr<Device> Ptr;

    Device(Alsa::Api::Ptr api, const AlsaIdentifier& id)
      : Api(api)
      , Pcm(api, id)
      , CanPause(false)
      , ChangeSign(false)
    {
    }

    ~Device()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {
      }
    }

    void SetParameters(unsigned buffersCount, const RenderParameters& params)
    {
      const boost::shared_ptr<snd_pcm_hw_params_t> hwParams = Allocate<snd_pcm_hw_params_t>(Api,
        &Alsa::Api::snd_pcm_hw_params_malloc, &Alsa::Api::snd_pcm_hw_params_free);
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params_any, hwParams.get(), THIS_LINE);
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params_set_access, hwParams.get(), SND_PCM_ACCESS_RW_INTERLEAVED, THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting resampling possibility");
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params_set_rate_resample, hwParams.get(), 1u, THIS_LINE);

      const boost::shared_ptr<snd_pcm_format_mask_t> fmtMask = Allocate<snd_pcm_format_mask_t>(Api,
        &Alsa::Api::snd_pcm_format_mask_malloc, &Alsa::Api::snd_pcm_format_mask_free);
      Api->snd_pcm_hw_params_get_format_mask(hwParams.get(), fmtMask.get());

      const SoundFormat fmt(*Api, fmtMask.get());
      Log::Debug(THIS_MODULE, "Setting format");
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params_set_format, hwParams.get(), fmt.Get(), THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting channels");
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params_set_channels, hwParams.get(), unsigned(OUTPUT_CHANNELS), THIS_LINE);
      const unsigned samplerate = params.SoundFreq();
      Log::Debug(THIS_MODULE, "Setting frequency to %1%", samplerate);
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params_set_rate, hwParams.get(), samplerate, 0, THIS_LINE);
      Log::Debug(THIS_MODULE, "Setting buffers count to %1%", buffersCount);
      int dir = 0;
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params_set_periods_near, hwParams.get(), &buffersCount, &dir, THIS_LINE);
      Log::Debug(THIS_MODULE, "Actually set to %1%", buffersCount);

      snd_pcm_uframes_t minBufSize(buffersCount * params.SamplesPerFrame());
      Log::Debug(THIS_MODULE, "Setting buffer size to %1% samples", minBufSize);
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params_set_buffer_size_near, hwParams.get(), &minBufSize, THIS_LINE);
      Log::Debug(THIS_MODULE, "Actually set %1% samples", minBufSize);

      Log::Debug(THIS_MODULE, "Applying parameters");
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_hw_params, hwParams.get(), THIS_LINE);
      Pcm.CheckedCall(&Alsa::Api::snd_pcm_prepare, THIS_LINE);

      CanPause = Api->snd_pcm_hw_params_can_pause(hwParams.get()) != 0;
      Log::Debug(THIS_MODULE, CanPause ? "Hardware support pause" : "Hardware doesn't support pause");
      ChangeSign = fmt.ChangeSign();
    }

    void Close()
    {
      Pcm.Close();
      CanPause = ChangeSign = false;
    }

    void Write(Chunk& buffer)
    {
      if (ChangeSign)
      {
        std::transform(buffer.front().begin(), buffer.back().end(), buffer.front().begin(), &ToSignedSample);
      }
      Pcm.Write(buffer);
    }
    
    void Pause()
    {
      if (CanPause)
      {
        Pcm.CheckedCall(&Alsa::Api::snd_pcm_pause, 1, THIS_LINE);
      }
    }
    
    void Resume()
    {
      if (CanPause)
      {
        Pcm.CheckedCall(&Alsa::Api::snd_pcm_pause, 0, THIS_LINE);
      }
    }
  private:
    const Alsa::Api::Ptr Api;
    PCMDevice Pcm;
    bool CanPause;
    bool ChangeSign;
  };

  class MixerElementsIterator
  {
  public:
    MixerElementsIterator(Alsa::Api::Ptr api, snd_mixer_t& mixer)
      : Api(api)
      , Current(Api->snd_mixer_first_elem(&mixer))
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
    
    String GetName() const
    {
      return FromStdString(Api->snd_mixer_selem_get_name(Current));
    }

    void Next()
    {
      Current = Api->snd_mixer_elem_next(Current);
      SkipNotsupported();
    }
  private:
    void SkipNotsupported()
    {
      while (Current)
      {
        const snd_mixer_elem_type_t type = Api->snd_mixer_elem_get_type(Current);
        if (type == SND_MIXER_ELEM_SIMPLE &&
            Api->snd_mixer_selem_has_playback_volume(Current) != 0)
        {
          break;
        }
        Current = Api->snd_mixer_elem_next(Current);
      }
    }
  private:
    const Alsa::Api::Ptr Api;
    snd_mixer_elem_t* Current;
  };

  class MixerDevice : public AutoHandle<snd_mixer_t>
  {
  public:
    MixerDevice(Alsa::Api::Ptr api, const String& card)
      : AutoHandle<snd_mixer_t>(api, card)
    {
      Open();
    }
    
    ~MixerDevice()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {
      }
    }
    
    void Open()
    {
      Log::Debug(THIS_MODULE, "Opening mixer device '%1%'", Name);
      CheckResult(Api->snd_mixer_open(&Handle, 0), THIS_LINE);
    }
    
    void Close()
    {
      if (Handle)
      {
        Log::Debug(THIS_MODULE, "Closing mixer device '%1%'", Name);
        CheckResult(Api->snd_mixer_close(Release()), THIS_LINE);
      }
    }
  };

  class AttachedMixer
  {
  public:
    AttachedMixer(Alsa::Api::Ptr api, const String& card)
      : Api(api)
      , Name(IO::ConvertToFilename(card))
      , MixDev(api, card)
    {
      Open();
    }
    
    ~AttachedMixer()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {
      }
    }

    void Open()
    {
      Log::Debug(THIS_MODULE, "Attaching to mixer device '%1%'", Name);
      MixDev.CheckedCall(&Alsa::Api::snd_mixer_attach, Name.c_str(), THIS_LINE);
      MixDev.CheckedCall(&Alsa::Api::snd_mixer_selem_register, static_cast<snd_mixer_selem_regopt*>(0), static_cast<snd_mixer_class_t**>(0), THIS_LINE);
      MixDev.CheckedCall(&Alsa::Api::snd_mixer_load, THIS_LINE);
    }

    void Close()
    {
      if (MixDev.Get())
      {
        Log::Debug(THIS_MODULE, "Detaching from mixer device '%1%'", Name);
        MixDev.CheckedCall(&Alsa::Api::snd_mixer_detach, Name.c_str(), THIS_LINE);
        MixDev.Close();
      }
    }
    
    MixerElementsIterator GetElements() const
    {
      Require(MixDev.Get());
      return MixerElementsIterator(Api, *MixDev.Get());
    }
  private:
    const Alsa::Api::Ptr Api;
    const std::string Name;
    MixerDevice MixDev;
  };

  class Mixer
  {
  public:
    typedef boost::shared_ptr<Mixer> Ptr;
    
    Mixer(Alsa::Api::Ptr api, const AlsaIdentifier& id, const String& mixer)
      : Api(api)
      , Attached(api, id.GetCard())
      , MixerElement(0)
    {

      Log::Debug(THIS_MODULE, "Opening mixer '%1%'", mixer);
      //find mixer element
      for (MixerElementsIterator iter = Attached.GetElements(); iter.IsValid(); iter.Next())
      {
        snd_mixer_elem_t* const elem = iter.Get();
        const String mixName = iter.GetName();
        Log::Debug(THIS_MODULE, "Checking for mixer %1%", mixName);
        if (mixer.empty())
        {
          Log::Debug(THIS_MODULE, "Using first mixer: %1%", mixName);
          MixerElement = elem;
          break;
        }
        else if (mixer == mixName)
        {
          Log::Debug(THIS_MODULE, "Found mixer: %1%", mixName);
          MixerElement = elem;
          break;
        }
      }
      if (!MixerElement)
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_ALSA_BACKEND_NO_MIXER, mixer);
      }
    }

    ~Mixer()
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
      MixerElement = 0;
      Attached.Close();
    }

    Error GetVolume(MultiGain& volume) const
    {
      if (!MixerElement)
      {
        volume = MultiGain();
        return Error();
      }
      try
      {
        BOOST_STATIC_ASSERT(MultiGain::static_size == 2);
        long minVol = 0, maxVol = 0;
        CheckResult(*Api, Api->snd_mixer_selem_get_playback_volume_range(MixerElement, &minVol, &maxVol), THIS_LINE);
        const long volRange = maxVol - minVol;

        long leftVol = 0, rightVol = 0;
        CheckResult(*Api, Api->snd_mixer_selem_get_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_LEFT, &leftVol), THIS_LINE);
        CheckResult(*Api, Api->snd_mixer_selem_get_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_RIGHT, &rightVol), THIS_LINE);
        volume[0] = Gain(leftVol - minVol) / volRange;
        volume[1] = Gain(rightVol - minVol) / volRange;
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    Error SetVolume(const MultiGain& volume)
    {
      if (volume.end() != std::find_if(volume.begin(), volume.end(), std::bind2nd(std::greater<Gain>(), Gain(1.0))))
      {
        return Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_GAIN);
      }
      if (!MixerElement)
      {
        return Error();
      }
      try
      {
        BOOST_STATIC_ASSERT(MultiGain::static_size == 2);
        long minVol = 0, maxVol = 0;
        CheckResult(*Api, Api->snd_mixer_selem_get_playback_volume_range(MixerElement, &minVol, &maxVol), THIS_LINE);
        const long volRange = maxVol - minVol;

        const long leftVol = static_cast<long>(volume[0] * volRange) + minVol;
        const long rightVol = static_cast<long>(volume[1] * volRange) + minVol;
        CheckResult(*Api, Api->snd_mixer_selem_set_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_LEFT, leftVol), THIS_LINE);
        CheckResult(*Api, Api->snd_mixer_selem_set_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_RIGHT, rightVol), THIS_LINE);

        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }
  private:
    const Alsa::Api::Ptr Api;
    AttachedMixer Attached;
    snd_mixer_elem_t* MixerElement;
  };

  class AlsaVolumeControl : public VolumeControl
  {
  public:
    explicit AlsaVolumeControl(Mixer::Ptr mixer)
      : Mix(mixer)
    {
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      Log::Debug(THIS_MODULE, "GetVolume");
      if (Mixer::Ptr obj = Mix.lock())
      {
        return obj->GetVolume(volume);
      }
      Log::Debug(THIS_MODULE, "Volume control is expired");
      return Error();
    }

    virtual Error SetVolume(const MultiGain& volume)
    {
      Log::Debug(THIS_MODULE, "SetVolume");
      if (Mixer::Ptr obj = Mix.lock())
      {
        return obj->SetVolume(volume);
      }
      Log::Debug(THIS_MODULE, "Volume control is expired");
      return Error();
    }
  private:
    const boost::weak_ptr<Mixer> Mix;
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
    AlsaBackendWorker(Alsa::Api::Ptr api, Parameters::Accessor::Ptr params)
      : Api(api)
      , BackendParams(params)
      , RenderingParameters(RenderParameters::Create(BackendParams))
    {
    }

    virtual ~AlsaBackendWorker()
    {
      assert(!Objects.Dev || !"AlsaBackend was destroyed without stopping");
    }

    virtual void Test()
    {
      const AlsaObjects obj = OpenDevices();
      obj.Dev->Close();
      obj.Mix->Close();
      Log::Debug(THIS_MODULE, "Checked!");
    }

    virtual void OnStartup(const Module::Holder& /*module*/)
    {
      Log::Debug(THIS_MODULE, "Starting");
      Objects = OpenDevices();
      Log::Debug(THIS_MODULE, "Started");
    }

    virtual void OnShutdown()
    {
      Log::Debug(THIS_MODULE, "Stopping");
      Objects.Vol.reset();
      Objects.Mix.reset();
      Objects.Dev.reset();
      Log::Debug(THIS_MODULE, "Stopped");
    }

    virtual void OnPause()
    {
      Objects.Dev->Pause();
    }

    virtual void OnResume()
    {
      Objects.Dev->Resume();
    }

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }

    virtual void OnBufferReady(Chunk& buffer)
    {
      Objects.Dev->Write(buffer);
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return CreateVolumeControlDelegate(Objects.Vol);
    }
  private:
    struct AlsaObjects
    {
      Device::Ptr Dev;
      Mixer::Ptr Mix;
      VolumeControl::Ptr Vol;
    };
    
    AlsaObjects OpenDevices() const
    {
      const AlsaBackendParameters params(*BackendParams);
      const AlsaIdentifier deviceId(params.GetDeviceName());

      AlsaObjects res;
      res.Dev = boost::make_shared<Device>(Api, deviceId);
      res.Dev->SetParameters(params.GetBuffersCount(), *RenderingParameters);
      res.Mix = boost::make_shared<Mixer>(Api, deviceId, params.GetMixerName());
      res.Vol = boost::make_shared<AlsaVolumeControl>(res.Mix);
      return res;
    }
  private:
    const Alsa::Api::Ptr Api;
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    AlsaObjects Objects;
  };

  class AlsaBackendCreator : public BackendCreator
  {
  public:
    explicit AlsaBackendCreator(Alsa::Api::Ptr api)
      : Api(api)
    {
    }
    
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
        const BackendWorker::Ptr worker(new AlsaBackendWorker(Api, allParams));
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
  private:
    const Alsa::Api::Ptr Api;
  };

  boost::shared_ptr<snd_ctl_t> OpenDevice(Alsa::Api::Ptr api, const std::string& deviceName)
  {
    snd_ctl_t* ctl = 0;
    return api->snd_ctl_open(&ctl, deviceName.c_str(), 0) >= 0
      ? boost::shared_ptr<snd_ctl_t>(ctl, boost::bind(&Alsa::Api::snd_ctl_close, api, _1))
      : boost::shared_ptr<snd_ctl_t>();
  }

  class CardsIterator
  {
  public:
    explicit CardsIterator(Alsa::Api::Ptr api)
      : Api(api)
      , Index(-1)
    {
      Next();
    }

    bool IsValid() const
    {
      return CurHandle;
    }

    snd_ctl_t& Handle() const
    {
      return *CurHandle;
    }

    std::string Name() const
    {
      return CurName;
    }

    std::string Id() const
    {
      return CurId;
    }

    void Next()
    {
      CurHandle = boost::shared_ptr<snd_ctl_t>();
      CurName.clear();
      CurId.clear();
      const boost::shared_ptr<snd_ctl_card_info_t> cardInfo = Allocate<snd_ctl_card_info_t>(Api,
        &Alsa::Api::snd_ctl_card_info_malloc, &Alsa::Api::snd_ctl_card_info_free);

      for (; Api->snd_card_next(&Index) >= 0 && Index >= 0; )
      {
        const std::string hwId = (boost::format("hw:%i") % Index).str();
        const boost::shared_ptr<snd_ctl_t> handle = OpenDevice(Api, hwId);
        if (!handle)
        {
          continue;
        }
        if (Api->snd_ctl_card_info(handle.get(), cardInfo.get()) < 0)
        {
          continue;
        }
        CurHandle = handle;
        CurId = Api->snd_ctl_card_info_get_id(cardInfo.get());
        CurName = Api->snd_ctl_card_info_get_name(cardInfo.get());
        return;
      }
    }
  private:
    const Alsa::Api::Ptr Api;
    int Index;
    boost::shared_ptr<snd_ctl_t> CurHandle;
    std::string CurName;
    std::string CurId;
  };

  class PCMDevicesIterator
  {
  public:
    PCMDevicesIterator(Alsa::Api::Ptr api, const CardsIterator& card)
      : Api(api)
      , Card(card)
      , Index(-1)
    {
      Next();
    }

    bool IsValid() const
    {
      return !CurName.empty();
    }

    std::string Name() const
    {
      return CurName;
    }

    std::string Id() const
    {
      return (boost::format("hw:%s,%u") % Card.Id() % Index).str();
    }

    void Next()
    {
      CurName.clear();
      const boost::shared_ptr<snd_pcm_info_t> pcmInfo = Allocate<snd_pcm_info_t>(Api,
        &Alsa::Api::snd_pcm_info_malloc, &Alsa::Api::snd_pcm_info_free);
      for (; Api->snd_ctl_pcm_next_device(&Card.Handle(), &Index) >= 0 && Index >= 0; )
      {
        Api->snd_pcm_info_set_device(pcmInfo.get(), Index);
        Api->snd_pcm_info_set_subdevice(pcmInfo.get(), 0);
        Api->snd_pcm_info_set_stream(pcmInfo.get(), SND_PCM_STREAM_PLAYBACK);
        if (Api->snd_ctl_pcm_info(&Card.Handle(), pcmInfo.get()) < 0)
        {
          continue;
        }
        CurName = Api->snd_pcm_info_get_name(pcmInfo.get());
        return;
      }
    }

    void Reset()
    {
      Index = -1;
      Next();
    }
  private:
    const Alsa::Api::Ptr Api;
    const CardsIterator& Card;
    int Index;
    std::string CurName;
  };

  class AlsaDevice : public ALSA::Device
  {
  public:
    AlsaDevice(Alsa::Api::Ptr api, const String& id, const String& name, const String& cardName)
      : Api(api)
      , IdValue(id)
      , NameValue(name)
      , CardNameValue(cardName)
    {
    }

    virtual String Id() const
    {
      return IdValue;
    }

    virtual String Name() const
    {
      return NameValue;
    }

    virtual String CardName() const
    {
      return CardNameValue;
    }

    virtual StringArray Mixers() const
    {
      try
      {
        const AlsaIdentifier id(IdValue);
        const AttachedMixer attached(Api, id.GetCard());
        StringArray result;
        for (MixerElementsIterator iter(attached.GetElements()); iter.IsValid(); iter.Next())
        {
          const String mixName = iter.GetName();
          result.push_back(mixName);
        }
        return result;
      }
      catch (const Error&)
      {
        return StringArray();
      }
    }

    static Ptr CreateDefault(Alsa::Api::Ptr api)
    {
      return boost::make_shared<AlsaDevice>(api,
        Parameters::ZXTune::Sound::Backends::ALSA::DEVICE_DEFAULT,
        Text::ALSA_BACKEND_DEFAULT_DEVICE,
        Text::ALSA_BACKEND_DEFAULT_DEVICE
        );
    }

    static Ptr Create(Alsa::Api::Ptr api, const CardsIterator& card, const PCMDevicesIterator& dev)
    {
      return boost::make_shared<AlsaDevice>(api,
        FromStdString(dev.Id()), FromStdString(dev.Name()), FromStdString(card.Name())
        );
    }

  private:
    const Alsa::Api::Ptr Api;
    const String IdValue;
    const String NameValue;
    const String CardNameValue;
  };

  class AlsaDevicesIterator : public ALSA::Device::Iterator
  {
  public:
    explicit AlsaDevicesIterator(Alsa::Api::Ptr api)
      : Api(api)
      , Cards(api)
      , Devices(api, Cards)
      , Current(AlsaDevice::CreateDefault(api))
    {
    }

    virtual bool IsValid() const
    {
      return Current;
    }

    virtual ALSA::Device::Ptr Get() const
    {
      return Current;
    }

    virtual void Next()
    {
      Current = ALSA::Device::Ptr();
      while (Cards.IsValid())
      {
        for (; Devices.IsValid(); Devices.Next())
        {
          Current = AlsaDevice::Create(Api, Cards, Devices);
          Devices.Next();
          return;
        }
        Cards.Next();
        if (Cards.IsValid())
        {
          Devices.Reset();
        }
      }
    }
  private:
    const Alsa::Api::Ptr Api;
    CardsIterator Cards;
    PCMDevicesIterator Devices;
    ALSA::Device::Ptr Current;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAlsaBackend(BackendsEnumerator& enumerator)
    {
      try
      {
        const Alsa::Api::Ptr api = Alsa::LoadDynamicApi();
        Log::Debug(THIS_MODULE, "Detected ALSA %1%", api->snd_asoundlib_version());
        const BackendCreator::Ptr creator(new AlsaBackendCreator(api));
        enumerator.RegisterCreator(creator);
      }
      catch (const Error& e)
      {
        Log::Debug(THIS_MODULE, "%1%", Error::ToString(e));
      }
    }

    namespace ALSA
    {
      Device::Iterator::Ptr EnumerateDevices()
      {
        try
        {
          /*
            Note: api instance should be created using cache for every instance of backend (TODO).
            Call of snd_config_update_free_global should be performed on destruction of Api's insance,
            but till alsa 1.0.24 this will lead to crash.
            TODO: check for previously called snd_lib_error_set_handler(NULL)
          */
          const Alsa::Api::Ptr api = Alsa::LoadDynamicApi();
          return Device::Iterator::Ptr(new AlsaDevicesIterator(api));
        }
        catch (const Error& e)
        {
          Log::Debug(THIS_MODULE, "%1%", Error::ToString(e));
          return Device::Iterator::CreateStub();
        }
      }
    }
  }
}
