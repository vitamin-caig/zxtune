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
#include <debug_log.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/noncopyable.hpp>
#include <boost/thread/condition_variable.hpp>
//text includes
#include <sound/text/backends.h>

#define FILE_TAG 608CF986

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Debug::Stream Dbg("Sound::Backend::Sdl");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM;

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  class SdlBackendParameters
  {
  public:
    explicit SdlBackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    uint_t GetBuffersCount() const
    {
      Parameters::IntType val = Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS, val) &&
          (!in_range<Parameters::IntType>(val, BUFFERS_MIN, BUFFERS_MAX)))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("SDL backend error: buffers count (%1%) is out of range (%2%..%3%)."), static_cast<int_t>(val), BUFFERS_MIN, BUFFERS_MAX);
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
      Dbg("Change buffers count %1% -> %2%", Buffers.size(), size);
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
      , Queue(Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS_DEFAULT)
    {
      if (0 == WasInitialized)
      {
        Dbg("Initializing");
        CheckCall(Api->SDL_Init(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Dbg("Initializing sound subsystem");
        CheckCall(Api->SDL_InitSubSystem(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
    }

    virtual ~SdlBackendWorker()
    {
      if (0 == WasInitialized)
      {
        Dbg("Shutting down");
        Api->SDL_Quit();
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Dbg("Shutting down sound subsystem");
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
      Dbg("Starting playback");

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
      const SdlBackendParameters params(*BackendParams);
      Queue.SetSize(params.GetBuffersCount());
      CheckCall(Api->SDL_OpenAudio(&format, 0) >= 0, THIS_LINE);
      Api->SDL_PauseAudio(0);
    }

    virtual void OnShutdown()
    {
      Dbg("Shutdown");
      Api->SDL_CloseAudio();
    }

    virtual void OnPause()
    {
      Dbg("Pause");
      Api->SDL_PauseAudio(1);
    }

    virtual void OnResume()
    {
      Dbg("Resume");
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
          throw MakeFormattedError(loc,
            translate("Error in SDL backend: %1%."), FromStdString(txt));
        }
        throw Error(loc, translate("Unknown error in SDL backend."));
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

  const String ID = Text::SDL_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("SDL support backend");

  class SdlBackendCreator : public BackendCreator
  {
  public:
    explicit SdlBackendCreator(Sdl::Api::Ptr api)
      : Api(api)
    {
    }

    virtual String Id() const
    {
      return ID;
    }

    virtual String Description() const
    {
      return translate(DESCRIPTION);
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
        return MakeFormattedError(THIS_LINE,
          translate("Failed to create backend '%1%'."), Id()).AddSuberror(e);
      }
    }
  private:
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
        Dbg("Detected SDL %1%.%2%.%3%", unsigned(vers->major), unsigned(vers->minor), unsigned(vers->patch));
        const BackendCreator::Ptr creator(new SdlBackendCreator(api));
        enumerator.RegisterCreator(creator);
      }
      catch (const Error& e)
      {
        enumerator.RegisterCreator(CreateUnavailableBackendStub(ID, DESCRIPTION, CAPABILITIES, e));
      }
    }
  }
}
