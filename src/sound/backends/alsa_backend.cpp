/**
 *
 * @file
 *
 * @brief  ALSA backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/alsa.h"
#include "sound/backends/backend_impl.h"
#include "sound/backends/gates/alsa_api.h"
#include "sound/backends/l10n.h"
#include "sound/backends/storage.h"
#include "sound/backends/volume_control.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <debug/log.h>
#include <math/numeric.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
#include <strings/split.h>
// std includes
#include <utility>

namespace Sound::Alsa
{
  const Debug::Stream Dbg("Sound::Backend::Alsa");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;

  const uint_t LATENCY_MIN = 20;
  const uint_t LATENCY_MAX = 10000;

  inline void CheckResult(Api& api, int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, translate("Error in ALSA backend: {}."), api.snd_strerror(res));
    }
  }

  class SoundFormat
  {
  public:
    SoundFormat(Api& api, snd_pcm_format_mask_t* mask)
      : Native(GetSoundFormat(Sample::MID == 0))
      , Negated(GetSoundFormat(Sample::MID != 0))
      , NativeSupported(api.snd_pcm_format_mask_test(mask, Native))
      , NegatedSupported(api.snd_pcm_format_mask_test(mask, Negated))
    {}

    bool IsSupported() const
    {
      return NativeSupported || NegatedSupported;
    }

    snd_pcm_format_t Get() const
    {
      return NativeSupported ? Native : (NegatedSupported ? Negated : SND_PCM_FORMAT_UNKNOWN);
    }

  private:
    static snd_pcm_format_t GetSoundFormat(bool isSigned)
    {
      switch (Sample::BITS)
      {
      case 8:
        return isSigned ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_U8;
      case 16:
        return isSigned ? (isLE() ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_S16_BE)
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
  class AutoHandle
  {
  public:
    AutoHandle(Api::Ptr api, StringView name)
      : AlsaApi(std::move(api))
      , Name(name)
      , Handle(nullptr)
    {}

    AutoHandle(const AutoHandle&) = delete;

    T* Get() const
    {
      return Handle;
    }

    void CheckedCall(int (Api::*func)(T*), Error::LocationRef loc) const
    {
      CheckResult(((*AlsaApi).*func)(Handle), loc);
    }

    template<class P1>
    void CheckedCall(int (Api::*func)(T*, P1), P1 p1, Error::LocationRef loc) const
    {
      CheckResult(((*AlsaApi).*func)(Handle, p1), loc);
    }

    template<class P1, class P2>
    void CheckedCall(int (Api::*func)(T*, P1, P2), P1 p1, P2 p2, Error::LocationRef loc) const
    {
      CheckResult(((*AlsaApi).*func)(Handle, p1, p2), loc);
    }

    template<class P1, class P2, class P3>
    void CheckedCall(int (Api::*func)(T*, P1, P2, P3), P1 p1, P2 p2, P3 p3, Error::LocationRef loc) const
    {
      CheckResult(((*AlsaApi).*func)(Handle, p1, p2, p3), loc);
    }

    template<class P1, class P2, class P3, class P4, class P5, class P6>
    void CheckedCall(int (Api::*func)(T*, P1, P2, P3, P4, P5, P6), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6,
                     Error::LocationRef loc) const
    {
      CheckResult(((*AlsaApi).*func)(Handle, p1, p2, p3, p4, p5, p6), loc);
    }

  protected:
    T* Release()
    {
      T* tmp = nullptr;
      std::swap(tmp, Handle);
      Name.clear();
      return tmp;
    }

    void CheckResult(int res, Error::LocationRef loc) const
    {
      if (res < 0)
      {
        throw MakeFormattedError(loc, translate("Error in ALSA backend while working with device '{0}': {1}."), Name,
                                 AlsaApi->snd_strerror(res));
      }
    }

  protected:
    const Api::Ptr AlsaApi;
    String Name;
    T* Handle;
  };

  class Identifier
  {
  public:
    explicit Identifier(StringView id)
    {
      auto elements = Strings::Split(id, ":,"sv);
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

  // Use the most atomic operations to prevent resources leak in case of error
  class PCMDevice : public AutoHandle<snd_pcm_t>
  {
  public:
    PCMDevice(Api::Ptr api, const Identifier& id)
      : AutoHandle<snd_pcm_t>(std::move(api), id.GetPCM())
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
      {}
    }

    void Open()
    {
      Require(Handle == nullptr);
      Dbg("Opening PCM device '{}'", Name);
      CheckResult(AlsaApi->snd_pcm_open(&Handle, Name.c_str(), SND_PCM_STREAM_PLAYBACK, 0), THIS_LINE);
    }

    void Close()
    {
      if (Handle)
      {
        // do not break if error while drain- we need to close
        AlsaApi->snd_pcm_drain(Handle);
        AlsaApi->snd_pcm_hw_free(Handle);
        Dbg("Closing PCM device '{}'", Name);
        CheckResult(AlsaApi->snd_pcm_close(Release()), THIS_LINE);
      }
    }

    void Write(const Chunk& buffer)
    {
      const Sample* data = buffer.data();
      std::size_t size = buffer.size();
      while (size)
      {
        snd_pcm_sframes_t res = AlsaApi->snd_pcm_writei(Handle, data, size);
        if (res < 0)
        {
          res = AlsaApi->snd_pcm_recover(Handle, res, 1);
        }
        if (res < 0)
        {
          CheckedCall(&Api::snd_pcm_prepare, THIS_LINE);
          continue;
        }
        data += res;
        size -= res;
      }
    }
  };

  template<class T>
  std::shared_ptr<T> Allocate(Api::Ptr api, int (Api::*allocFunc)(T**), void (Api::*freeFunc)(T*))
  {
    T* res = nullptr;
    CheckResult(*api, ((*api).*allocFunc)(&res), THIS_LINE);
    return res ? std::shared_ptr<T>(res, [api = std::move(api), freeFunc](auto&& arg) { ((*api).*freeFunc)(arg); })
               : std::shared_ptr<T>();
  }

  class DeviceWrapper
  {
  public:
    using Ptr = std::shared_ptr<DeviceWrapper>;

    DeviceWrapper(Api::Ptr api, const Identifier& id)
      : AlsaApi(std::move(api))
      , Pcm(AlsaApi, id)
    {}

    ~DeviceWrapper()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {}
    }

    void SetParameters(Time::Milliseconds lat, const RenderParameters& params)
    {
      const auto hwParams =
          Allocate<snd_pcm_hw_params_t>(AlsaApi, &Api::snd_pcm_hw_params_malloc, &Api::snd_pcm_hw_params_free);
      Pcm.CheckedCall(&Api::snd_pcm_hw_params_any, hwParams.get(), THIS_LINE);

      const bool canPause = AlsaApi->snd_pcm_hw_params_can_pause(hwParams.get()) != 0;
      Dbg(canPause ? "Hardware support pause" : "Hardware doesn't support pause");

      const auto fmtMask =
          Allocate<snd_pcm_format_mask_t>(AlsaApi, &Api::snd_pcm_format_mask_malloc, &Api::snd_pcm_format_mask_free);
      AlsaApi->snd_pcm_hw_params_get_format_mask(hwParams.get(), fmtMask.get());

      const SoundFormat fmt(*AlsaApi, fmtMask.get());
      if (!fmt.IsSupported())
      {
        throw Error(THIS_LINE, translate("No suitable formats supported by ALSA."));
      }

      const unsigned freq = params.SoundFreq();
      const unsigned latency = Time::Microseconds(lat).Get();
      Dbg("Setting parameters: rate={}Hz latency={}uS", freq, latency);
      Pcm.CheckedCall(&Api::snd_pcm_set_params, fmt.Get(), SND_PCM_ACCESS_RW_INTERLEAVED, unsigned(Sample::CHANNELS),
                      freq, 1, latency, THIS_LINE);

      Pcm.CheckedCall(&Api::snd_pcm_prepare, THIS_LINE);

      CanPause = canPause;
      Format = fmt.Get();
    }

    void Close()
    {
      Pcm.Close();
      CanPause = false;
      Format = SND_PCM_FORMAT_UNKNOWN;
    }

    void Write(Chunk& buffer)
    {
      switch (Format)
      {
      case SND_PCM_FORMAT_S16_LE:
      case SND_PCM_FORMAT_S16_BE:
        buffer.ToS16();
        break;
      case SND_PCM_FORMAT_U8:
        buffer.ToU8();
        break;
      default:
        assert(!"Unsupported format");
        break;
      }
      Pcm.Write(buffer);
    }

    void Pause()
    {
      if (CanPause)
      {
        Pcm.CheckedCall(&Api::snd_pcm_pause, 1, THIS_LINE);
      }
    }

    void Resume()
    {
      if (CanPause)
      {
        Pcm.CheckedCall(&Api::snd_pcm_pause, 0, THIS_LINE);
      }
    }

  private:
    const Api::Ptr AlsaApi;
    PCMDevice Pcm;
    bool CanPause = false;
    snd_pcm_format_t Format = SND_PCM_FORMAT_UNKNOWN;
  };

  class MixerElementsIterator
  {
  public:
    MixerElementsIterator(Api::Ptr api, snd_mixer_t& mixer)
      : AlsaApi(std::move(api))
      , Current(AlsaApi->snd_mixer_first_elem(&mixer))
    {
      SkipNotsupported();
    }

    bool IsValid() const
    {
      return Current != nullptr;
    }

    snd_mixer_elem_t* Get() const
    {
      return Current;
    }

    String GetName() const
    {
      return AlsaApi->snd_mixer_selem_get_name(Current);
    }

    void Next()
    {
      Current = AlsaApi->snd_mixer_elem_next(Current);
      SkipNotsupported();
    }

  private:
    void SkipNotsupported()
    {
      while (Current)
      {
        const snd_mixer_elem_type_t type = AlsaApi->snd_mixer_elem_get_type(Current);
        if (type == SND_MIXER_ELEM_SIMPLE && AlsaApi->snd_mixer_selem_has_playback_volume(Current) != 0)
        {
          break;
        }
        Current = AlsaApi->snd_mixer_elem_next(Current);
      }
    }

  private:
    const Api::Ptr AlsaApi;
    snd_mixer_elem_t* Current;
  };

  class MixerDevice : public AutoHandle<snd_mixer_t>
  {
  public:
    MixerDevice(Api::Ptr api, StringView card)
      : AutoHandle<snd_mixer_t>(std::move(api), card)
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
      {}
    }

    void Open()
    {
      Dbg("Opening mixer device '{}'", Name);
      CheckResult(AlsaApi->snd_mixer_open(&Handle, 0), THIS_LINE);
    }

    void Close()
    {
      if (Handle)
      {
        Dbg("Closing mixer device '{}'", Name);
        CheckResult(AlsaApi->snd_mixer_close(Release()), THIS_LINE);
      }
    }
  };

  class AttachedMixer
  {
  public:
    AttachedMixer(Api::Ptr api, StringView card)
      : AlsaApi(std::move(api))
      , Name(card)
      , MixDev(AlsaApi, card)
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
      {}
    }

    void Open()
    {
      Dbg("Attaching to mixer device '{}'", Name);
      MixDev.CheckedCall(&Api::snd_mixer_attach, Name.c_str(), THIS_LINE);
      MixDev.CheckedCall(&Api::snd_mixer_selem_register, static_cast<snd_mixer_selem_regopt*>(nullptr),
                         static_cast<snd_mixer_class_t**>(nullptr), THIS_LINE);
      MixDev.CheckedCall(&Api::snd_mixer_load, THIS_LINE);
    }

    void Close()
    {
      if (MixDev.Get())
      {
        Dbg("Detaching from mixer device '{}'", Name);
        MixDev.CheckedCall(&Api::snd_mixer_detach, Name.c_str(), THIS_LINE);
        MixDev.Close();
      }
    }

    MixerElementsIterator GetElements() const
    {
      Require(MixDev.Get());
      return {AlsaApi, *MixDev.Get()};
    }

  private:
    const Api::Ptr AlsaApi;
    const String Name;
    MixerDevice MixDev;
  };

  class Mixer
  {
  public:
    using Ptr = std::shared_ptr<Mixer>;

    Mixer(Api::Ptr api, const Identifier& id, StringView mixer)
      : AlsaApi(std::move(api))
      , Attached(AlsaApi, id.GetCard())
    {

      Dbg("Opening mixer '{}'", mixer);
      // find mixer element
      for (MixerElementsIterator iter = Attached.GetElements(); iter.IsValid(); iter.Next())
      {
        snd_mixer_elem_t* const elem = iter.Get();
        const String mixName = iter.GetName();
        Dbg("Checking for mixer {}", mixName);
        if (mixer.empty())
        {
          Dbg("Using first mixer: {}", mixName);
          MixerElement = elem;
          break;
        }
        else if (mixer == mixName)
        {
          Dbg("Found mixer: {}", mixName);
          MixerElement = elem;
          break;
        }
      }
      if (!MixerElement)
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to found mixer '{}' for ALSA backend."), mixer);
      }
    }

    ~Mixer()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {}
    }

    void Close()
    {
      MixerElement = nullptr;
      Attached.Close();
    }

    Gain GetVolume() const
    {
      if (!MixerElement)
      {
        return {};
      }
      else
      {
        static_assert(Gain::CHANNELS == 2, "Invalid channels count");
        long minVol = 0;
        long maxVol = 0;
        CheckResult(*AlsaApi, AlsaApi->snd_mixer_selem_get_playback_volume_range(MixerElement, &minVol, &maxVol),
                    THIS_LINE);
        const long volRange = maxVol - minVol;

        long leftVol = 0;
        long rightVol = 0;
        CheckResult(*AlsaApi,
                    AlsaApi->snd_mixer_selem_get_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_LEFT, &leftVol),
                    THIS_LINE);
        CheckResult(*AlsaApi,
                    AlsaApi->snd_mixer_selem_get_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_RIGHT, &rightVol),
                    THIS_LINE);
        return {Gain::Type(leftVol - minVol, volRange), Gain::Type(rightVol - minVol, volRange)};
      }
    }

    void SetVolume(const Gain& volume)
    {
      if (!volume.IsNormalized())
      {
        throw Error(THIS_LINE, translate("Failed to set volume: gain is out of range."));
      }
      if (MixerElement)
      {
        static_assert(Gain::CHANNELS == 2, "Invalid channels count");
        long minVol = 0;
        long maxVol = 0;
        CheckResult(*AlsaApi, AlsaApi->snd_mixer_selem_get_playback_volume_range(MixerElement, &minVol, &maxVol),
                    THIS_LINE);
        const long volRange = maxVol - minVol;

        const long leftVol = static_cast<long>((volume.Left() * volRange).Round() + minVol);
        const long rightVol = static_cast<long>((volume.Right() * volRange).Round() + minVol);
        CheckResult(*AlsaApi,
                    AlsaApi->snd_mixer_selem_set_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_LEFT, leftVol),
                    THIS_LINE);
        CheckResult(*AlsaApi,
                    AlsaApi->snd_mixer_selem_set_playback_volume(MixerElement, SND_MIXER_SCHN_FRONT_RIGHT, rightVol),
                    THIS_LINE);
      }
    }

  private:
    const Api::Ptr AlsaApi;
    AttachedMixer Attached;
    snd_mixer_elem_t* MixerElement = nullptr;
  };

  class VolumeControl : public Sound::VolumeControl
  {
  public:
    explicit VolumeControl(const Mixer::Ptr& mixer)
      : Mix(mixer)
    {}

    Gain GetVolume() const override
    {
      Dbg("GetVolume");
      if (const Mixer::Ptr obj = Mix.lock())
      {
        return obj->GetVolume();
      }
      Dbg("Volume control is expired");
      return {};
    }

    void SetVolume(const Gain& volume) override
    {
      Dbg("SetVolume");
      if (const Mixer::Ptr obj = Mix.lock())
      {
        return obj->SetVolume(volume);
      }
      Dbg("Volume control is expired");
    }

  private:
    const std::weak_ptr<Mixer> Mix;
  };

  class BackendParameters
  {
  public:
    explicit BackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {}

    String GetDeviceName() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
      return Parameters::GetString(Accessor, DEVICE, DEVICE_DEFAULT);
    }

    String GetMixerName() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
      return Parameters::GetString(Accessor, MIXER);
    }

    Time::Milliseconds GetLatency() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
      const auto val = Parameters::GetInteger<uint_t>(Accessor, LATENCY, LATENCY_DEFAULT);
      if (!Math::InRange(val, LATENCY_MIN, LATENCY_MAX))
      {
        throw MakeFormattedError(THIS_LINE, translate("ALSA backend error: latency ({0}) is out of range ({1}..{2})."),
                                 val, LATENCY_MIN, LATENCY_MAX);
      }
      return Time::Milliseconds{val};
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Api::Ptr api, Parameters::Accessor::Ptr params)
      : AlsaApi(std::move(api))
      , Params(std::move(params))
    {}

    ~BackendWorker() override
    {
      assert(!Objects.Dev || !"AlsaBackend was destroyed without stopping");
    }

    void Startup() override
    {
      Dbg("Starting");
      Objects = OpenDevices();
      Dbg("Started");
    }

    void Shutdown() override
    {
      Dbg("Stopping");
      Objects.Vol.reset();
      Objects.Mix.reset();
      Objects.Dev.reset();
      Dbg("Stopped");
    }

    void Pause() override
    {
      Objects.Dev->Pause();
    }

    void Resume() override
    {
      Objects.Dev->Resume();
    }

    void FrameStart(const Module::State& /*state*/) override {}

    void FrameFinish(Chunk buffer) override
    {
      Objects.Dev->Write(buffer);
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return CreateVolumeControlDelegate(Objects.Vol);
    }

  private:
    struct AlsaObjects
    {
      DeviceWrapper::Ptr Dev;
      Mixer::Ptr Mix;
      VolumeControl::Ptr Vol;
    };

    AlsaObjects OpenDevices() const
    {
      const RenderParameters::Ptr sound = RenderParameters::Create(Params);
      const BackendParameters backend(*Params);
      const Identifier deviceId(backend.GetDeviceName());

      AlsaObjects res;
      res.Dev = MakePtr<DeviceWrapper>(AlsaApi, deviceId);
      res.Dev->SetParameters(backend.GetLatency(), *sound);
      res.Mix = MakePtr<Mixer>(AlsaApi, deviceId, backend.GetMixerName());
      res.Vol = MakePtr<VolumeControl>(res.Mix);
      return res;
    }

  private:
    const Api::Ptr AlsaApi;
    const Parameters::Accessor::Ptr Params;
    AlsaObjects Objects;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : AlsaApi(std::move(api))
    {}

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr /*holder*/) const override
    {
      return MakePtr<BackendWorker>(AlsaApi, params);
    }

  private:
    const Api::Ptr AlsaApi;
  };

  std::shared_ptr<snd_ctl_t> OpenDevice(Api::Ptr api, const String& deviceName)
  {
    snd_ctl_t* ctl = nullptr;
    return api->snd_ctl_open(&ctl, deviceName.c_str(), 0) >= 0
               ? std::shared_ptr<snd_ctl_t>(ctl, [api = std::move(api)](auto&& arg) { api->snd_ctl_close(arg); })
               : std::shared_ptr<snd_ctl_t>();
  }

  class CardsIterator
  {
  public:
    explicit CardsIterator(Api::Ptr api)
      : AlsaApi(std::move(api))
    {
      Next();
    }

    bool IsValid() const
    {
      return CurHandle.get();
    }

    snd_ctl_t& Handle() const
    {
      return *CurHandle;
    }

    String Name() const
    {
      return CurName;
    }

    String Id() const
    {
      return CurId;
    }

    void Next()
    {
      CurHandle = std::shared_ptr<snd_ctl_t>();
      CurName.clear();
      CurId.clear();
      const auto cardInfo =
          Allocate<snd_ctl_card_info_t>(AlsaApi, &Api::snd_ctl_card_info_malloc, &Api::snd_ctl_card_info_free);

      for (; AlsaApi->snd_card_next(&Index) >= 0 && Index >= 0;)
      {
        const auto hwId = Strings::Format("hw:{}", Index);
        const auto handle = OpenDevice(AlsaApi, hwId);
        if (!handle)
        {
          continue;
        }
        if (AlsaApi->snd_ctl_card_info(handle.get(), cardInfo.get()) < 0)
        {
          continue;
        }
        CurHandle = handle;
        CurId = AlsaApi->snd_ctl_card_info_get_id(cardInfo.get());
        CurName = AlsaApi->snd_ctl_card_info_get_name(cardInfo.get());
        return;
      }
    }

  private:
    const Api::Ptr AlsaApi;
    int Index = -1;
    std::shared_ptr<snd_ctl_t> CurHandle;
    String CurName;
    String CurId;
  };

  class DevicesIterator
  {
  public:
    DevicesIterator(Api::Ptr api, const CardsIterator& card)
      : AlsaApi(std::move(api))
      , Card(card)

    {
      Next();
    }

    bool IsValid() const
    {
      return !CurName.empty();
    }

    String Name() const
    {
      return CurName;
    }

    String Id() const
    {
      return Strings::Format("hw:{},{}", Card.Id(), Index);
    }

    void Next()
    {
      CurName.clear();
      const auto pcmInfo = Allocate<snd_pcm_info_t>(AlsaApi, &Api::snd_pcm_info_malloc, &Alsa::Api::snd_pcm_info_free);
      for (; Card.IsValid() && AlsaApi->snd_ctl_pcm_next_device(&Card.Handle(), &Index) >= 0 && Index >= 0;)
      {
        AlsaApi->snd_pcm_info_set_device(pcmInfo.get(), Index);
        AlsaApi->snd_pcm_info_set_subdevice(pcmInfo.get(), 0);
        AlsaApi->snd_pcm_info_set_stream(pcmInfo.get(), SND_PCM_STREAM_PLAYBACK);
        if (AlsaApi->snd_ctl_pcm_info(&Card.Handle(), pcmInfo.get()) < 0)
        {
          continue;
        }
        CurName = AlsaApi->snd_pcm_info_get_name(pcmInfo.get());
        return;
      }
    }

    void Reset()
    {
      Index = -1;
      Next();
    }

  private:
    const Api::Ptr AlsaApi;
    const CardsIterator& Card;
    int Index = -1;
    String CurName;
  };

  class DeviceInfo : public Device
  {
  public:
    DeviceInfo(Api::Ptr api, StringView id, StringView name, StringView cardName)
      : AlsaApi(std::move(api))
      , IdValue(id)
      , NameValue(name)
      , CardNameValue(cardName)
    {}

    String Id() const override
    {
      return IdValue;
    }

    String Name() const override
    {
      return NameValue;
    }

    String CardName() const override
    {
      return CardNameValue;
    }

    Strings::Array Mixers() const override
    {
      try
      {
        const Identifier id(IdValue);
        const AttachedMixer attached(AlsaApi, id.GetCard());
        Strings::Array result;
        for (MixerElementsIterator iter(attached.GetElements()); iter.IsValid(); iter.Next())
        {
          const String mixName = iter.GetName();
          result.push_back(mixName);
        }
        return result;
      }
      catch (const Error&)
      {
        return {};
      }
    }

    static Ptr CreateDefault(Api::Ptr api)
    {
      static const auto DEFAULT_DEVICE_NAME = "Default"sv;
      return MakePtr<DeviceInfo>(std::move(api), Parameters::ZXTune::Sound::Backends::Alsa::DEVICE_DEFAULT,
                                 DEFAULT_DEVICE_NAME, DEFAULT_DEVICE_NAME);
    }

    static Ptr Create(Api::Ptr api, const CardsIterator& card, const DevicesIterator& dev)
    {
      return MakePtr<DeviceInfo>(std::move(api), dev.Id(), dev.Name(), card.Name());
    }

  private:
    const Api::Ptr AlsaApi;
    const String IdValue;
    const String NameValue;
    const String CardNameValue;
  };

  class DeviceInfoIterator : public Device::Iterator
  {
  public:
    explicit DeviceInfoIterator(Api::Ptr api)
      : AlsaApi(std::move(api))
      , Cards(AlsaApi)
      , Devices(AlsaApi, Cards)
      , Current(Cards.IsValid() && Devices.IsValid() ? DeviceInfo::CreateDefault(AlsaApi) : Device::Ptr())
    {}

    bool IsValid() const override
    {
      return Current.get();
    }

    Device::Ptr Get() const override
    {
      return Current;
    }

    void Next() override
    {
      Current = Device::Ptr();
      while (Cards.IsValid())
      {
        for (; Devices.IsValid(); Devices.Next())
        {
          Current = DeviceInfo::Create(AlsaApi, Cards, Devices);
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
    const Api::Ptr AlsaApi;
    CardsIterator Cards;
    DevicesIterator Devices;
    Device::Ptr Current;
  };
}  // namespace Sound::Alsa

namespace Sound
{
  void RegisterAlsaBackend(BackendsStorage& storage)
  {
    try
    {
      auto api = Alsa::LoadDynamicApi();
      Alsa::Dbg("Detected Alsa {}", api->snd_asoundlib_version());
      if (Alsa::DeviceInfoIterator(api).IsValid())
      {
        auto factory = MakePtr<Alsa::BackendWorkerFactory>(std::move(api));
        storage.Register(Alsa::BACKEND_ID, Alsa::BACKEND_DESCRIPTION, Alsa::CAPABILITIES, std::move(factory));
      }
      else
      {
        throw Error(THIS_LINE, translate("No suitable output devices found"));
      }
    }
    catch (const Error& e)
    {
      storage.Register(Alsa::BACKEND_ID, Alsa::BACKEND_DESCRIPTION, Alsa::CAPABILITIES, e);
    }
  }

  namespace Alsa
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
        auto api = LoadDynamicApi();
        return MakePtr<DeviceInfoIterator>(std::move(api));
      }
      catch (const Error& e)
      {
        Alsa::Dbg("{}", e.ToString());
        return Device::Iterator::CreateStub();
      }
    }
  }  // namespace Alsa
}  // namespace Sound
