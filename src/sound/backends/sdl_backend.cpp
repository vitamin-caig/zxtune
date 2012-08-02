/*
Abstract:
  Sdl backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#define DECLSPEC

//local includes
#include "sdl_api.h"
#include "backend_impl.h"
#include "enumerator.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/noncopyable.hpp>
#include <boost/thread/condition_variable.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 608CF986

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::Sdl");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM;

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  class SDLBackendParameters
  {
  public:
    explicit SDLBackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    uint_t GetBuffersCount() const
    {
      Parameters::IntType val = Parameters::ZXTune::Sound::Backends::SDL::BUFFERS_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Sound::Backends::SDL::BUFFERS, val) &&
          (!in_range<Parameters::IntType>(val, BUFFERS_MIN, BUFFERS_MAX)))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_SDL_BACKEND_INVALID_BUFFERS, static_cast<int_t>(val), BUFFERS_MIN, BUFFERS_MAX);
      }
      return static_cast<uint_t>(val);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class BuffersQueue
  {
  public:
    explicit BuffersQueue(uint_t size)
      : Buffers(size)
      , FillIter(&Buffers.front(), &Buffers.back() + 1)
      , PlayIter(FillIter)
    {
    }

    void AddData(Chunk& buffer)
    {
      boost::mutex::scoped_lock locker(BufferMutex);
      while (FillIter->BytesToPlay)
      {
        PlayedEvent.wait(locker);
      }
      FillIter->Data.swap(buffer);
      FillIter->BytesToPlay = FillIter->Data.size() * sizeof(FillIter->Data.front());
      ++FillIter;
      FilledEvent.notify_one();
    }

    void GetData(uint8_t* stream, uint_t len)
    {
      boost::mutex::scoped_lock locker(BufferMutex);
      while (len)
      {
        //wait for data
        while (!PlayIter->BytesToPlay)
        {
          FilledEvent.wait(locker);
        }
        const uint_t inBuffer = PlayIter->Data.size() * sizeof(PlayIter->Data.front());
        const uint_t toCopy = std::min<uint_t>(len, PlayIter->BytesToPlay);
        const uint8_t* const src = safe_ptr_cast<const uint8_t*>(&PlayIter->Data.front());
        std::memcpy(stream, src + (inBuffer - PlayIter->BytesToPlay), toCopy);
        PlayIter->BytesToPlay -= toCopy;
        stream += toCopy;
        len -= toCopy;
        if (!PlayIter->BytesToPlay)
        {
          //buffer is played
          ++PlayIter;
          PlayedEvent.notify_one();
        }
      }
    }

    void SetSize(uint_t size)
    {
      Log::Debug(THIS_MODULE, "Change buffers count %1% -> %2%", Buffers.size(), size);
      Buffers.resize(size);
      FillIter = CycledIterator<Buffer*>(&Buffers.front(), &Buffers.back() + 1);
      PlayIter = FillIter;
    }

    uint_t GetSize() const
    {
      return Buffers.size();
    }
  private:
    boost::mutex BufferMutex;
    boost::condition_variable FilledEvent, PlayedEvent;
    struct Buffer
    {
      Buffer() : BytesToPlay()
      {
      }
      uint_t BytesToPlay;
      Chunk Data;
    };
    std::vector<Buffer> Buffers;
    CycledIterator<Buffer*> FillIter, PlayIter;
  };

  class SdlBackendWorker : public BackendWorker
                         , private boost::noncopyable
  {
  public:
    SdlBackendWorker(Sdl::Api::Ptr api, Parameters::Accessor::Ptr params)
      : Api(api)
      , BackendParams(params)
      , RenderingParameters(RenderParameters::Create(BackendParams))
      , WasInitialized(Api->SDL_WasInit(SDL_INIT_EVERYTHING))
      , Queue(Parameters::ZXTune::Sound::Backends::SDL::BUFFERS_DEFAULT)
    {
      if (0 == WasInitialized)
      {
        Log::Debug(THIS_MODULE, "Initializing");
        CheckCall(Api->SDL_Init(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Log::Debug(THIS_MODULE, "Initializing sound subsystem");
        CheckCall(Api->SDL_InitSubSystem(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
    }

    virtual ~SDLBackendWorker()
    {
      if (0 == WasInitialized)
      {
        Log::Debug(THIS_MODULE, "Shutting down");
        Api->SDL_Quit();
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Log::Debug(THIS_MODULE, "Shutting down sound subsystem");
        Api->SDL_QuitSubSystem(SDL_INIT_AUDIO);
      }
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
    }

    virtual void Test()
    {
      //TODO: implement
    }

    virtual void OnStartup(const Module::Holder& /*module*/)
    {
      Log::Debug(THIS_MODULE, "Starting playback");

      SDL_AudioSpec format;
      format.format = -1;
      switch (sizeof(Sample))
      {
      case 1:
        format.format = SAMPLE_SIGNED ? AUDIO_S8 : AUDIO_U8;
        break;
      case 2:
        format.format = SAMPLE_SIGNED ? AUDIO_S16SYS : AUDIO_U16SYS;
        break;
      default:
        assert(!"Invalid format");
      }

      format.freq = RenderingParameters->SoundFreq();
      format.channels = static_cast< ::Uint8>(OUTPUT_CHANNELS);
      format.samples = RenderingParameters->SamplesPerFrame();
      //fix if size is not power of 2
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
      const SDLBackendParameters params(*BackendParams);
      Queue.SetSize(params.GetBuffersCount());
      CheckCall(Api->SDL_OpenAudio(&format, 0) >= 0, THIS_LINE);
      Api->SDL_PauseAudio(0);
    }

    virtual void OnShutdown()
    {
      Log::Debug(THIS_MODULE, "Shutdown");
      Api->SDL_CloseAudio();
    }

    virtual void OnPause()
    {
      Log::Debug(THIS_MODULE, "Pause");
      Api->SDL_PauseAudio(1);
    }

    virtual void OnResume()
    {
      Log::Debug(THIS_MODULE, "Resume");
      Api->SDL_PauseAudio(0);
    }

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }


    virtual void OnBufferReady(Chunk& buffer)
    {
      Queue.AddData(buffer);
    }
  private:
    void CheckCall(bool ok, Error::LocationRef loc) const
    {
      if (!ok)
      {
        if (const char* txt = Api->SDL_GetError())
        {
          throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
            Text::SOUND_ERROR_SDL_BACKEND_ERROR, FromStdString(txt));
        }
        throw Error(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_SDL_BACKEND_UNKNOWN_ERROR);
      }
    }

    static void OnBuffer(void* param, ::Uint8* stream, int len)
    {
      BuffersQueue* const queue = static_cast<BuffersQueue*>(param);
      queue->GetData(stream, len);
    }
  private:
    const Sdl::Api::Ptr Api;
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    const ::Uint32 WasInitialized;
    BuffersQueue Queue;
  };

  class SdlBackendCreator : public BackendCreator
  {
  public:
    explicit SdlBackendCreator(Sdl::Api::Ptr api)
      : Api(api)
    {
    }

    virtual String Id() const
    {
      return Text::SDL_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::SDL_BACKEND_DESCRIPTION;
    }

    virtual uint_t Capabilities() const
    {
      return CAPABILITIES;
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const BackendWorker::Ptr worker(new SdlBackendWorker(Api, allParams));
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
  const Sdl::Api::Ptr Api;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterSdlBackend(BackendsEnumerator& enumerator)
    {
      try
      {
        const Sdl::Api::Ptr api = Sdl::LoadDynamicApi();
        const SDL_version* const vers = api->SDL_Linked_Version();
        Log::Debug(THIS_MODULE, "Detected SDL %1%.%2%.%3%", unsigned(vers->major), unsigned(vers->minor), unsigned(vers->patch));
        const BackendCreator::Ptr creator(new SdlBackendCreator(api));
        enumerator.RegisterCreator(creator);
      }
      catch (const Error& e)
      {
        enumerator.RegisterCreator(CreateUnavailableBackendStub(Text::SDL_BACKEND_ID, Text::SDL_BACKEND_DESCRIPTION, CAPABILITIES, e));
      }
    }
  }
}
