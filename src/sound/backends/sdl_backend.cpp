/**
 *
 * @file
 *
 * @brief  SDL backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#define DECLSPEC

#include "sound/backends/backend_impl.h"
#include "sound/backends/gates/sdl_api.h"
#include "sound/backends/l10n.h"
#include "sound/backends/sdl.h"
#include "sound/backends/storage.h"

#include "debug/log.h"
#include "math/numeric.h"
#include "sound/backends_parameters.h"
#include "sound/render_params.h"
#include "sound/sound_parameters.h"

#include "byteorder.h"
#include "error_tools.h"
#include "make_ptr.h"

#include <condition_variable>
#include <utility>

namespace Sound::Sdl
{
  const Debug::Stream Dbg("Sound::Backend::Sdl");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM;

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  class BackendParameters
  {
  public:
    explicit BackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {}

    uint_t GetBuffersCount() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Sdl;
      const auto val = Parameters::GetInteger(Accessor, BUFFERS, BUFFERS_DEFAULT);
      if (!Math::InRange(val, BUFFERS_MIN, BUFFERS_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
                                 translate("SDL backend error: buffers count ({0}) is out of range ({1}..{2})."), val,
                                 BUFFERS_MIN, BUFFERS_MAX);
      }
      return val;
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  class BuffersQueue
  {
  public:
    explicit BuffersQueue(uint_t size)
      : Buffers(size)
      , FillIter(Buffers.data(), Buffers.data() + Buffers.size())
      , PlayIter(FillIter)
    {}

    void AddData(Chunk& buffer)
    {
      std::unique_lock<std::mutex> lock(BufferMutex);
      while (FillIter->BytesToPlay)
      {
        PlayedEvent.wait(lock);
      }
      FillIter->Data.swap(buffer);
      FillIter->BytesToPlay = FillIter->Data.size() * sizeof(FillIter->Data.front());
      ++FillIter;
      FilledEvent.notify_one();
    }

    void GetData(uint8_t* stream, uint_t len)
    {
      std::unique_lock<std::mutex> lock(BufferMutex);
      while (len)
      {
        // wait for data
        while (!PlayIter->BytesToPlay)
        {
          FilledEvent.wait(lock);
        }
        const uint_t inBuffer = PlayIter->Data.size() * sizeof(PlayIter->Data.front());
        const uint_t toCopy = std::min<uint_t>(len, PlayIter->BytesToPlay);
        const auto* const src = safe_ptr_cast<const uint8_t*>(PlayIter->Data.data());
        std::memcpy(stream, src + (inBuffer - PlayIter->BytesToPlay), toCopy);
        PlayIter->BytesToPlay -= toCopy;
        stream += toCopy;
        len -= toCopy;
        if (!PlayIter->BytesToPlay)
        {
          // buffer is played
          ++PlayIter;
          PlayedEvent.notify_one();
        }
      }
    }

    void SetSize(uint_t size)
    {
      Dbg("Change buffers count {} -> {}", Buffers.size(), size);
      Buffers.resize(size);
      FillIter = CycledIterator<Buffer*>(Buffers.data(), Buffers.data() + Buffers.size());
      PlayIter = FillIter;
    }

    uint_t GetSize() const
    {
      return Buffers.size();
    }

  private:
    std::mutex BufferMutex;
    std::condition_variable FilledEvent, PlayedEvent;
    struct Buffer
    {
      Buffer() = default;
      uint_t BytesToPlay = 0;
      Chunk Data;
    };
    std::vector<Buffer> Buffers;
    CycledIterator<Buffer*> FillIter, PlayIter;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Api::Ptr api, Parameters::Accessor::Ptr params)
      : SdlApi(std::move(api))
      , Params(std::move(params))
      , WasInitialized(SdlApi->SDL_WasInit(SDL_INIT_EVERYTHING))
      , Queue(Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS_DEFAULT)
    {
      if (0 == WasInitialized)
      {
        Dbg("Initializing");
        CheckCall(SdlApi->SDL_Init(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Dbg("Initializing sound subsystem");
        CheckCall(SdlApi->SDL_InitSubSystem(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
    }

    ~BackendWorker() override
    {
      if (0 == WasInitialized)
      {
        Dbg("Shutting down");
        SdlApi->SDL_Quit();
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Dbg("Shutting down sound subsystem");
        SdlApi->SDL_QuitSubSystem(SDL_INIT_AUDIO);
      }
    }

    void Startup() override
    {
      Dbg("Starting playback");

      SDL_AudioSpec format;
      format.format = -1;
      const bool sampleSigned = Sample::MID == 0;
      switch (Sample::BITS)
      {
      case 8:
        format.format = sampleSigned ? AUDIO_S8 : AUDIO_U8;
        break;
      case 16:
        format.format = sampleSigned ? AUDIO_S16SYS : AUDIO_U16SYS;
        break;
      default:
        assert(!"Invalid format");
      }

      const RenderParameters::Ptr sound = RenderParameters::Create(Params);
      const BackendParameters backend(*Params);
      format.freq = sound->SoundFreq();
      format.channels = static_cast< ::Uint8>(Sample::CHANNELS);
      format.samples = format.freq / 2;  // keep 0.5 seconds of data
      // fix if size is not power of 2
      if (0 != (format.samples & (format.samples - 1)))
      {
        unsigned msk = 1;
        while (format.samples > msk)
        {
          msk <<= 1;
        }
        format.samples = msk;
      }
      format.callback = OnBuffer;
      format.userdata = &Queue;
      Queue.SetSize(backend.GetBuffersCount());
      CheckCall(SdlApi->SDL_OpenAudio(&format, nullptr) >= 0, THIS_LINE);
      SdlApi->SDL_PauseAudio(0);
    }

    void Shutdown() override
    {
      Dbg("Shutdown");
      SdlApi->SDL_CloseAudio();
    }

    void Pause() override
    {
      Dbg("Pause");
      SdlApi->SDL_PauseAudio(1);
    }

    void Resume() override
    {
      Dbg("Resume");
      SdlApi->SDL_PauseAudio(0);
    }

    void FrameStart(const Module::State& /*state*/) override {}

    void FrameFinish(Chunk buffer) override
    {
      Queue.AddData(buffer);
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return {};
    }

  private:
    void CheckCall(bool ok, Error::LocationRef loc) const
    {
      if (!ok)
      {
        if (const char* txt = SdlApi->SDL_GetError())
        {
          throw MakeFormattedError(loc, translate("Error in SDL backend: {}."), txt);
        }
        throw Error(loc, translate("Unknown error in SDL backend."));
      }
    }

    static void OnBuffer(void* param, ::Uint8* stream, int len)
    {
      auto* const queue = static_cast<BuffersQueue*>(param);
      queue->GetData(stream, len);
    }

  private:
    const Api::Ptr SdlApi;
    const Parameters::Accessor::Ptr Params;
    const ::Uint32 WasInitialized;
    BuffersQueue Queue;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : SdlApi(std::move(api))
    {}

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr /*holder*/) const override
    {
      return MakePtr<BackendWorker>(SdlApi, params);
    }

  private:
    const Api::Ptr SdlApi;
  };
}  // namespace Sound::Sdl

namespace Sound
{
  void RegisterSdlBackend(BackendsStorage& storage)
  {
    try
    {
      auto api = Sdl::LoadDynamicApi();
      const SDL_version* const vers = api->SDL_Linked_Version();
      Sdl::Dbg("Detected SDL {}.{}.{}", unsigned(vers->major), unsigned(vers->minor), unsigned(vers->patch));
      auto factory = MakePtr<Sdl::BackendWorkerFactory>(std::move(api));
      storage.Register(Sdl::BACKEND_ID, Sdl::BACKEND_DESCRIPTION, Sdl::CAPABILITIES, std::move(factory));
    }
    catch (const Error& e)
    {
      storage.Register(Sdl::BACKEND_ID, Sdl::BACKEND_DESCRIPTION, Sdl::CAPABILITIES, e);
    }
  }
}  // namespace Sound
