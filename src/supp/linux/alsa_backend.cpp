#include "../sound_backend_impl.h"
#include "../sound_backend_types.h"

#include <tools.h>
#include <error.h>

#include <alsa/asoundlib.h>

#include <cassert>

#include <boost/bind.hpp>
#include <boost/thread.hpp>

//implementation
#include <boost/thread/exceptions.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>

#define FILE_TAG 8B5627E4

#include <iostream>

namespace
{
  using namespace ZXTune::Sound;

  const char DEFAULT_DEVICE[] = "default";
  const std::size_t MINIMAL_LATENCY = 1000;

  const std::size_t MIN_BUFFER_DEPTH = 2;
  const std::size_t MAX_BUFFER_DEPTH = 4;

  inline void CheckAlsa(int res)
  {
    if (res < 0)
    {
      std::cout << ::snd_strerror(res);
      throw Error(ERROR_DETAIL, 1, ::snd_strerror(res));
    }
  }

  class Event
  {
    typedef boost::unique_lock<boost::mutex> Locker;
  public:
    Event() : Mutex(), Condition()
    {
    }

    void Wait()
    {
      Locker LockObj(Mutex);
      Condition.wait(LockObj);
    }

    void Signal()
    {
      Condition.notify_all();
    }

  private:
    boost::mutex Mutex;
    boost::condition_variable Condition;
  };

  class AlsaBackend : public BackendImpl
  {
    class BufferDescr
    {
    public:
      BufferDescr(snd_pcm_t* handle, std::size_t sizeInSamples)
       : Handle(handle), Buffer(sizeInSamples), Size(sizeInSamples), Filled(false)
      {
      }
      BufferDescr(const BufferDescr& rh) : Handle(rh.Handle), Buffer(rh.Buffer.size()), Size(rh.Size), Filled(false)
      {
      }

      void Fill(const void* data, std::size_t sizeInSamples)
      {
        assert(!IsFilled());
        assert(sizeInSamples <= Buffer.size());
        Size = sizeInSamples;
        std::memcpy(&Buffer[0], data, Size * sizeof(Buffer.front()));
        Filled = true;
      }

      void Play()
      {
        assert(IsFilled());
        assert(0 != Handle);
        std::size_t toWrite(Size / OUTPUT_CHANNELS);
        const Sample* buf(&Buffer[0]);
        while (toWrite)
        {
          snd_pcm_sframes_t res(::snd_pcm_writei(Handle, buf, toWrite));
          if (res < 0)
          {
            if (!(res = ::snd_pcm_recover(Handle, res, 0)))
            {
              continue;
            }
          }
          CheckAlsa(res);
          toWrite -= res;
          buf += res * OUTPUT_CHANNELS;
        }
        Filled = false;
      }

      bool IsFilled() const
      {
        return Filled;
      }
    private:
      snd_pcm_t* Handle;
      std::vector<Sample> Buffer;
      std::size_t Size;
      volatile bool Filled;
    };

  public:
    AlsaBackend() : DevHandle(0)/*, MixHandle(0)*/, Stopping(false)
    {
    }

    virtual ~AlsaBackend()
    {
      try
      {
        Stop();
      }
      catch (...)
      {
        //TODO
      }
    }

    virtual void OnBufferReady(const void* data, std::size_t sizeInBytes)
    {
      while (FillPtr->IsFilled())
      {
        PlayedEvent.Wait();
      }
      FillPtr->Fill(data, sizeInBytes / sizeof(Sample));
      ++FillPtr;
      FilledEvent.Signal();
    }

    virtual void OnParametersChanged(unsigned changedFields)
    {
      const unsigned mask(DRIVER_PARAMS | DRIVER_FLAGS | BUFFER | SOUND_FREQ);
      if (changedFields & mask)
      {
        const bool needStartup(0 != DevHandle);
        OnShutdown();

        if (changedFields & (BUFFER | SOUND_FREQ | DRIVER_FLAGS))
        {
          if (changedFields & DRIVER_FLAGS)
          {
            const unsigned depth(Params.DriverFlags & BUFFER_DEPTH_MASK);
            if (depth &&
              (depth < MIN_BUFFER_DEPTH || depth > MAX_BUFFER_DEPTH))
            {
              throw 2;//TODO
            }
          }
        }
        if (needStartup)
        {
          OnStartup();
        }
      }
      else if (changedFields & PREAMP)
      {
        SetVolume();
      }
    }

    virtual void OnStartup()
    {
      assert(0 == DevHandle);
      CheckAlsa(::snd_pcm_open(&DevHandle,
        Params.DriverParameters.empty() ? DEFAULT_DEVICE : Params.DriverParameters.c_str(),
        SND_PCM_STREAM_PLAYBACK, 0));

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
      CheckAlsa(::snd_pcm_set_params(DevHandle, fmt, SND_PCM_ACCESS_RW_INTERLEAVED,
        OUTPUT_CHANNELS, Params.SoundParameters.SoundFreq, 0/*no resample*/, std::max(Params.BufferInMs * 1000, MINIMAL_LATENCY)));
      SetVolume();
      const unsigned depth(Params.DriverFlags & BUFFER_DEPTH_MASK);
      Buffers.assign(depth ? depth : MIN_BUFFER_DEPTH,
        BufferDescr(DevHandle, OUTPUT_CHANNELS * (Params.SoundParameters.SoundFreq * Params.BufferInMs / 1000)));
      PlayPtr = FillPtr = cycled_iterator<BufferDescr*>(&Buffers[0], &Buffers[Buffers.size()]);
      Stopping = false;
      PlaybackThread = boost::thread(
        std::mem_fun(&AlsaBackend::RenderFunc),
        this);
    }

    virtual void OnShutdown()
    {
      if (0 != DevHandle)
      {
        Stopping = true;
        PlaybackThread.join();
        CheckAlsa(::snd_pcm_close(DevHandle));
        DevHandle = 0;
        //assert(0 != MixHandle);
        //CheckResult(0 == ::close(MixHandle));
        //MixHandle = -1;
      }
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }
  private:
    void SetVolume()
    {
      int vol(100 * Params.Preamp / FIXED_POINT_PRECISION);
      vol |= vol << 16;
      //CheckResult(-1 != ::ioctl(MixHandle, SOUND_MIXER_WRITE_VOLUME, &vol));
    }

    void RenderFunc()
    {
      while (!Stopping)
      {
        while (!PlayPtr->IsFilled())
        {
          FilledEvent.Wait();
        }
        PlayPtr->Play();
        ++PlayPtr;
        PlayedEvent.Signal();
      }
    }

  private:
    snd_pcm_t* DevHandle;
    //int MixHandle;
    std::vector<BufferDescr> Buffers;
    cycled_iterator<BufferDescr*> FillPtr, PlayPtr;
    volatile bool Stopping;
    Event FilledEvent, PlayedEvent;
    boost::thread PlaybackThread;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Backend::Ptr CreateAlsaBackend()
    {
      return Backend::Ptr(new AlsaBackend);
    }
  }
}

