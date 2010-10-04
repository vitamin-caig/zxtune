/*
Abstract:
  SDL backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef SDL_SUPPORT

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
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
//platform-dependent includes
#include <SDL/SDL.h>
//boost includes
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 608CF986

namespace
{
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("SDL");

  const Char SDL_BACKEND_ID[] = {'s', 'd', 'l', 0};
  const String SDL_BACKEND_VERSION(FromStdString("$Rev$"));

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  void CheckCall(bool ok, Error::LocationRef loc)
  {
    if (!ok)
    {
      if (const char* txt = ::SDL_GetError())
      {
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
          Text::SOUND_ERROR_SDL_BACKEND_ERROR, FromStdString(txt));
      }
      throw Error(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_SDL_BACKEND_UNKNOWN_ERROR);
    }
  }

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
      if (Accessor.FindIntValue(Parameters::ZXTune::Sound::Backends::SDL::BUFFERS, val) &&
          (!in_range<Parameters::IntType>(val, BUFFERS_MIN, BUFFERS_MAX)))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_SDL_BACKEND_INVALID_BUFFERS, static_cast<int_t>(val), BUFFERS_MIN, BUFFERS_MAX);
      }
      return static_cast<uint_t>(val);
    }

    uint_t GetFrequency() const
    {
      Parameters::IntType res = Parameters::ZXTune::Sound::FREQUENCY_DEFAULT;
      Accessor.FindIntValue(Parameters::ZXTune::Sound::FREQUENCY, res);
      return static_cast<uint_t>(res);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class SDLBackend : public BackendImpl
                   , private boost::noncopyable
  {
  public:
    SDLBackend()
      : WasInitialized(::SDL_WasInit(SDL_INIT_EVERYTHING))
      , BuffersCount(Parameters::ZXTune::Sound::Backends::SDL::BUFFERS_DEFAULT)
      , Buffers(BuffersCount)
      , FillIter(&Buffers.front(), &Buffers.back() + 1)
      , PlayIter(FillIter)
    {
      if (0 == WasInitialized)
      {
        Log::Debug(THIS_MODULE, "Initializing");
        CheckCall(::SDL_Init(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Log::Debug(THIS_MODULE, "Initializing sound subsystem");
        CheckCall(::SDL_InitSubSystem(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
    }

    virtual ~SDLBackend()
    {
      if (0 == WasInitialized)
      {
        Log::Debug(THIS_MODULE, "Shutting down");
        ::SDL_Quit();
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Log::Debug(THIS_MODULE, "Shutting down sound subsystem");
        ::SDL_QuitSubSystem(SDL_INIT_AUDIO);
      }
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
    }

    virtual void OnStartup()
    {
      Locker lock(BackendMutex);
      DoStartup();
    }

    virtual void OnShutdown()
    {
      Locker lock(BackendMutex);
      ::SDL_CloseAudio();
    }

    virtual void OnPause()
    {
      Locker lock(BackendMutex);
      ::SDL_PauseAudio(1);
    }

    virtual void OnResume()
    {
      Locker lock(BackendMutex);
      ::SDL_PauseAudio(0);
    }

    virtual void OnParametersChanged(const Parameters::Accessor& updates)
    {
      const SDLBackendParameters curParams(updates);
      //check for parameters requires restarting
      const uint_t newBuffers = curParams.GetBuffersCount();
      const uint_t newFreq = curParams.GetFrequency();

      const bool buffersChanged = newBuffers != BuffersCount;
      const bool freqChanged = newFreq != RenderingParameters.SoundFreq;
      if (buffersChanged || freqChanged)
      {
        Locker lock(BackendMutex);
        const bool needStartup = SDL_AUDIO_STOPPED != ::SDL_GetAudioStatus();
        if (needStartup)
        {
          ::SDL_CloseAudio();
        }
        BuffersCount = newBuffers;

        if (needStartup)
        {
          DoStartup();
        }
      }
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      Locker lock(BackendMutex);
      boost::unique_lock<boost::mutex> locker(BufferMutex);
      while (FillIter->ToPlay)
      {
        PlayedEvent.wait(locker);
      }
      FillIter->Data.swap(buffer);
      FillIter->ToPlay = FillIter->Data.size() * sizeof(FillIter->Data.front());
      ++FillIter;
      FilledEvent.notify_one();
    }
  private:
    void DoStartup()
    {
      Log::Debug(THIS_MODULE, "Starting playback");

      SDL_AudioSpec format;
      format.format = -1;
      switch (sizeof(Sample))
      {
      case 1:
        format.format = AUDIO_S8;
        break;
      case 2:
        format.format = isLE() ? AUDIO_S16LSB : AUDIO_S16MSB;
        break;
      default:
        assert(!"Invalid format");
      }

      format.freq = static_cast<int>(RenderingParameters.SoundFreq);
      format.channels = static_cast< ::Uint8>(OUTPUT_CHANNELS);
      format.samples = BuffersCount * RenderingParameters.SamplesPerFrame();
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
      format.userdata = this;
      Buffers.resize(BuffersCount);
      FillIter = CycledIterator<Buffer*>(&Buffers.front(), &Buffers.back() + 1);
      PlayIter = FillIter;
      CheckCall(::SDL_OpenAudio(&format, 0) >= 0, THIS_LINE);
      ::SDL_PauseAudio(0);
    }

    static void OnBuffer(void* param, ::Uint8* stream, int len)
    {
      SDLBackend* const self = static_cast<SDLBackend*>(param);
      boost::unique_lock<boost::mutex> locker(self->BufferMutex);
      while (len)
      {
        //wait for data
        while (!self->PlayIter->ToPlay)
        {
          self->FilledEvent.wait(locker);
        }
        const uint_t inBuffer = self->PlayIter->Data.size() * sizeof(self->PlayIter->Data.front());
        const uint_t toCopy = std::min<uint_t>(len, self->PlayIter->ToPlay);
        const uint8_t* const src = safe_ptr_cast<const uint8_t*>(&self->PlayIter->Data.front());
        std::memcpy(stream, src + (inBuffer - self->PlayIter->ToPlay), toCopy);
        self->PlayIter->ToPlay -= toCopy;
        stream += toCopy;
        len -= toCopy;
        if (!self->PlayIter->ToPlay)
        {
          //buffer is played
          ++self->PlayIter;
          self->PlayedEvent.notify_one();
        }
      }
    }
  private:
    const ::Uint32 WasInitialized;
    boost::mutex BufferMutex;
    boost::condition_variable FilledEvent, PlayedEvent;
    struct Buffer
    {
      Buffer() : ToPlay()
      {
      }
      uint_t ToPlay;
      std::vector<MultiSample> Data;
    };
    uint_t BuffersCount;
    std::vector<Buffer> Buffers;
    CycledIterator<Buffer*> FillIter, PlayIter;
  };

  class SDLBackendCreator : public BackendCreator
                          , public boost::enable_shared_from_this<SDLBackendCreator>
  {
  public:
    virtual String Id() const
    {
      return SDL_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::SDL_BACKEND_DESCRIPTION;
    }

    virtual String Version() const
    {
      return SDL_BACKEND_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_SYSTEM;
    }

    virtual Error CreateBackend(const Parameters::Accessor& params, Backend::Ptr& result) const
    {
      const BackendInformation::Ptr info = shared_from_this();
      return SafeBackendWrapper<SDLBackend>::Create(info, params, result, THIS_LINE);
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterSDLBackend(BackendsEnumerator& enumerator)
    {
      const BackendCreator::Ptr creator(new SDLBackendCreator());
      enumerator.RegisterCreator(creator);
    }
  }
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterSDLBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}

#endif
