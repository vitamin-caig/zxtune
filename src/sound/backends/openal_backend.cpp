/**
 *
 * @file
 *
 * @brief  OpenAL backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/backend_impl.h"
#include "sound/backends/gates/openal_api.h"
#include "sound/backends/l10n.h"
#include "sound/backends/openal.h"
#include "sound/backends/storage.h"
#include "sound/backends/volume_control.h"
// common includes
#include <byteorder.h>
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <math/numeric.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
// std includes
#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace Sound::OpenAl
{
  const Debug::Stream Dbg("Sound::Backend::OpenAL");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM;

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  class ApiRef
  {
  public:
    explicit ApiRef(Api& api)
      : OalApi(api)
    {}

    ApiRef(const ApiRef&) = delete;

    void CheckError(Error::LocationRef loc) const
    {
      if (const ALenum err = OalApi.alGetError())
      {
        throw MakeFormattedError(loc, translate("Error in OpenAL backend: {}."), err);
      }
    }

    void RaiseError(Error::LocationRef loc) const
    {
      CheckError(loc);
      throw Error(loc, translate("Unknown error in OpenAL backend."));
    }

  protected:
    Api& OalApi;
  };

  class ActiveContext : private ApiRef
  {
  public:
    ActiveContext(Api& api, ALCdevice& device)
      : ApiRef(api)
      , Previous(OalApi.alcGetCurrentContext())
      , Current(OalApi.alcCreateContext(&device, nullptr), [&api](ALCcontext* ctx) { api.alcDestroyContext(ctx); })
    {
      Dbg("Create context instead of current {}", static_cast<void*>(Previous));
      if (!Current)
      {
        RaiseError(THIS_LINE);
      }
      Dbg("Set current context to {}", static_cast<void*>(Current.get()));
      if (!OalApi.alcMakeContextCurrent(Current.get()))
      {
        RaiseError(THIS_LINE);
      }
    }

    ~ActiveContext()
    {
      if (Current.get() == OalApi.alcGetCurrentContext())
      {
        Dbg("Restore previous context");
        if (!OalApi.alcMakeContextCurrent(Previous))
        {
          Dbg("Failed to deactivate context");
        }
      }
      else
      {
        Dbg("Context was lost");
      }
    }

  private:
    ALCcontext* const Previous;
    const std::shared_ptr<ALCcontext> Current;
  };

  class Buffers : private ApiRef
  {
  public:
    Buffers(Api& api, uint_t count)
      : ApiRef(api)
      , Format(GetFormat())
      , Size(count)
      , Ids(new ALuint[count])
    {
      Dbg("Generate {} buffers", count);
      OalApi.alGetError();
      OalApi.alGenBuffers(Size, Ids.get());
      CheckError(THIS_LINE);
    }

    ~Buffers()
    {
      Dbg("Release buffers");
      OalApi.alDeleteBuffers(Size, Ids.get());
    }

    ALuint* GetBuffers() const
    {
      return Ids.get();
    }

    void Fill(ALuint id, const Chunk& data, uint_t freq)
    {
      OalApi.alBufferData(id, Format, data.data(), data.size() * sizeof(data.front()), freq);
      CheckError(THIS_LINE);
    }

  private:
    static ALenum GetFormat()
    {
      static_assert(Sample::CHANNELS == 1 || Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sample::BITS == 8 || Sample::BITS == 16, "Incompatible sound sample bits count");
      switch (256 * Sample::CHANNELS + Sample::BITS)
      {
      case 0x108:
        return AL_FORMAT_MONO8;
      case 0x208:
        return AL_FORMAT_STEREO8;
      case 0x110:
        return AL_FORMAT_MONO16;
      case 0x210:
        return AL_FORMAT_STEREO16;
      default:
        return 0;
      }
    }

  private:
    const ALenum Format;
    const uint_t Size;
    const std::unique_ptr<ALuint[]> Ids;
  };

  class Source : private ApiRef
  {
  public:
    explicit Source(Api& api, uint_t freq, uint_t buffersCount)
      : ApiRef(api)
      , Freq(freq)
      , SleepPeriod(1)
    {
      Dbg("Create source");
      OalApi.alGetError();
      OalApi.alGenSources(1, &SrcId);
      CheckError(THIS_LINE);
      Queue = std::make_unique<Buffers>(OalApi, buffersCount);
      StartPlayback(buffersCount);
    }

    ~Source()
    {
      OalApi.alDeleteSources(1, &SrcId);
    }

    float GetGain() const
    {
      ALfloat val = 0;
      OalApi.alGetSourcef(SrcId, AL_GAIN, &val);
      return val;
    }

    void SetGain(float val)
    {
      OalApi.alSourcef(SrcId, AL_GAIN, val);
      CheckError(THIS_LINE);
    }

    void Play()
    {
      OalApi.alSourcePlay(SrcId);
      CheckError(THIS_LINE);
    }

    void Pause()
    {
      OalApi.alSourcePause(SrcId);
      CheckError(THIS_LINE);
    }

    void Write(const Chunk& data)
    {
      ALuint buf = GetFreeBuffer();
      Queue->Fill(buf, data, Freq);
      Send(&buf, 1);
    }

  private:
    void StartPlayback(uint_t buffersCount)
    {
      const Chunk empty(1);
      ALuint* const bufs = Queue->GetBuffers();
      for (uint_t idx = 0; idx != buffersCount; ++idx)
      {
        Queue->Fill(bufs[idx], empty, Freq);
      }
      Send(bufs, buffersCount);
    }

    ALuint GetFreeBuffer()
    {
      for (;;)
      {
        if (GetProperty(AL_BUFFERS_PROCESSED) != 0)
        {
          ALuint freeBuf = 0;
          OalApi.alSourceUnqueueBuffers(SrcId, 1, &freeBuf);
          CheckError(THIS_LINE);
          return freeBuf;
        }
        std::this_thread::sleep_for(SleepPeriod);
      }
    }

    void Send(ALuint* bufs, uint_t count)
    {
      OalApi.alSourceQueueBuffers(SrcId, count, bufs);
      CheckError(THIS_LINE);
      if (GetProperty(AL_SOURCE_STATE) != AL_PLAYING)
      {
        Play();
      }
    }

    ALint GetProperty(ALenum prop) const
    {
      ALint result = 0;
      OalApi.alGetSourcei(SrcId, prop, &result);
      CheckError(THIS_LINE);
      return result;
    }

  private:
    const uint_t Freq;
    const std::chrono::milliseconds SleepPeriod;
    ALuint SrcId;
    std::unique_ptr<Buffers> Queue;
  };

  class Device : private ApiRef
  {
  public:
    Device(Api& api, const String& deviceName)
      : ApiRef(api)
      , Dev(OalApi.alcOpenDevice(deviceName.empty() ? nullptr : deviceName.c_str()),
            [&api](ALCdevice* dev) { api.alcCloseDevice(dev); })
    {
      Dbg("Open device {}", deviceName);
      if (!Dev)
      {
        RaiseError(THIS_LINE);
      }
      Context = std::make_unique<ActiveContext>(OalApi, *Dev);
    }

  private:
    const std::shared_ptr<ALCdevice> Dev;
    std::unique_ptr<ActiveContext> Context;
  };

  class BackendParameters
  {
  public:
    explicit BackendParameters(Parameters::Accessor::Ptr accessor)
      : Accessor(*accessor)
      , RenderingParameters(RenderParameters::Create(std::move(accessor)))
    {}

    String GetDeviceName() const
    {
      Parameters::StringType strVal = Parameters::ZXTune::Sound::Backends::OpenAl::DEVICE_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::Sound::Backends::OpenAl::DEVICE, strVal);
      return strVal;
    }

    uint_t GetSoundFreq() const
    {
      return RenderingParameters->SoundFreq();
    }

    uint_t GetBuffersCount() const
    {
      Parameters::IntType val = Parameters::ZXTune::Sound::Backends::OpenAl::BUFFERS_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Sound::Backends::OpenAl::BUFFERS, val)
          && !Math::InRange<Parameters::IntType>(val, BUFFERS_MIN, BUFFERS_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
                                 translate("OpenAL backend error: buffers count ({0}) is out of range ({1}..{2})."),
                                 static_cast<int_t>(val), BUFFERS_MIN, BUFFERS_MAX);
      }
      return static_cast<uint_t>(val);
    }

  private:
    const Parameters::Accessor& Accessor;
    const RenderParameters::Ptr RenderingParameters;
  };

  class VolumeController : public VolumeControl
  {
  public:
    explicit VolumeController(Source& src)
      : Src(src)
    {}

    Gain GetVolume() const override
    {
      const Gain::Type val(Src.GetGain());
      return {val, val};
    }

    void SetVolume(const Gain& vol) override
    {
      const float val = ALfloat(vol.Left().Raw() + vol.Right().Raw()) / (2 * Gain::Type::PRECISION);
      Src.SetGain(val);
    }

  private:
    Source& Src;
  };

  struct State
  {
    State(Api& api, const BackendParameters& params)
      : Dev(new Device(api, params.GetDeviceName()))
      , Src(new Source(api, params.GetSoundFreq(), params.GetBuffersCount()))
      , Vol(new VolumeController(*Src))
    {}

    const std::unique_ptr<Device> Dev;
    const std::unique_ptr<Source> Src;
    const VolumeControl::Ptr Vol;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Api::Ptr api, Parameters::Accessor::Ptr params)
      : OalApi(std::move(api))
      , Params(std::move(params))
    {}

    void Startup() override
    {
      Dbg("Starting playback");

      const BackendParameters params(Params);
      Stat = std::make_unique<State>(*OalApi, params);
    }

    void Shutdown() override
    {
      Dbg("Shutdown");
      Stat.reset();
    }

    void Pause() override
    {
      Dbg("Pause");
      Stat->Src->Pause();
    }

    void Resume() override
    {
      Dbg("Resume");
      Stat->Src->Play();
    }

    void FrameStart(const Module::State& /*state*/) override {}

    void FrameFinish(Chunk buffer) override
    {
      Stat->Src->Write(buffer);
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return CreateVolumeControlDelegate(Stat->Vol);
    }

  private:
    const Api::Ptr OalApi;
    const Parameters::Accessor::Ptr Params;
    std::unique_ptr<State> Stat;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : OalApi(std::move(api))
    {}

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr /*holder*/) const override
    {
      return MakePtr<BackendWorker>(OalApi, params);
    }

  private:
    const Api::Ptr OalApi;
  };
}  // namespace Sound::OpenAl

namespace Sound
{
  void RegisterOpenAlBackend(BackendsStorage& storage)
  {
    try
    {
      auto api = OpenAl::LoadDynamicApi();
      const char* const version = api->alGetString(AL_VERSION);
      const char* const vendor = api->alGetString(AL_VENDOR);
      const char* const renderer = api->alGetString(AL_RENDERER);
      OpenAl::Dbg("Detected OpenAL v{} by '{}' (renderer '{}')", version, vendor,
                  renderer);  // usually empty strings...
      auto factory = MakePtr<OpenAl::BackendWorkerFactory>(std::move(api));
      storage.Register(OpenAl::BACKEND_ID, OpenAl::BACKEND_DESCRIPTION, OpenAl::CAPABILITIES, std::move(factory));
    }
    catch (const Error& e)
    {
      storage.Register(OpenAl::BACKEND_ID, OpenAl::BACKEND_DESCRIPTION, OpenAl::CAPABILITIES, e);
    }
  }

  namespace OpenAl
  {
    Strings::Array EnumerateDevices()
    {
      Strings::Array result;
      try
      {
        const Api::Ptr api = LoadDynamicApi();
        if (const char* str = api->alcGetString(nullptr, ALC_DEVICE_SPECIFIER))
        {
          while (*str)
          {
            const char* end = std::strchr(str, 0);
            result.push_back(String(str, end));
            str = end + 1;
          }
        }
      }
      catch (const Error&)
      {}
      return result;
    }
  }  // namespace OpenAl
}  // namespace Sound
