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
#include <sound/sound_parameters.h>
//platform-dependent includes
#include <SDL/SDL.h>
//boost includes
#include <boost/noncopyable.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 608CF986

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::SDL");

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
  private:
    const Parameters::Accessor& Accessor;
  };

  class SDLBackend : public BackendImpl
                   , private boost::noncopyable
  {
  public:
    explicit SDLBackend(CreateBackendParameters::Ptr params)
      : BackendImpl(params)
      , WasInitialized(::SDL_WasInit(SDL_INIT_EVERYTHING))
      , BuffersCount(Parameters::ZXTune::Sound::Backends::SDL::BUFFERS_DEFAULT)
      , Buffers(BuffersCount)
      , FillIter(&Buffers.front(), &Buffers.back() + 1)
      , PlayIter(FillIter)
      , Samplerate(0)
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
      OnParametersChanged(*SoundParameters);
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
      DoStartup();
    }

    virtual void OnShutdown()
    {
      ::SDL_CloseAudio();
    }

    virtual void OnPause()
    {
      ::SDL_PauseAudio(1);
    }

    virtual void OnResume()
    {
      ::SDL_PauseAudio(0);
    }

    void OnParametersChanged(const Parameters::Accessor& updates)
    {
      const SDLBackendParameters curParams(updates);
      //check for parameters requires restarting
      const uint_t newBuffers = curParams.GetBuffersCount();
      const uint_t newFreq = RenderingParameters->SoundFreq();

      const bool buffersChanged = newBuffers != BuffersCount;
      const bool freqChanged = newFreq != Samplerate;
      if (buffersChanged || freqChanged)
      {
        const bool needStartup = SDL_AUDIO_STOPPED != ::SDL_GetAudioStatus();
        if (needStartup)
        {
          ::SDL_CloseAudio();
        }
        BuffersCount = newBuffers;
        Samplerate = newFreq;

        if (needStartup)
        {
          DoStartup();
        }
      }
    }

    virtual void OnFrame()
    {
    }


    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
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

      format.freq = Samplerate;
      format.channels = static_cast< ::Uint8>(OUTPUT_CHANNELS);
      format.samples = BuffersCount * RenderingParameters->SamplesPerFrame();
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
    uint_t Samplerate;
  };

  class SDLBackendCreator : public BackendCreator
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

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      return SafeBackendWrapper<SDLBackend>::Create(Id(), params, result, THIS_LINE);
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
