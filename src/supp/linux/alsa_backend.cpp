#include "../sound_backend_async.h"

#include <tools.h>
#include <error.h>

#include <alsa/asoundlib.h>

#include <cassert>

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#define FILE_TAG 8B5627E4

#include <iostream>

namespace
{
  using namespace ZXTune::Sound;

  const char DEFAULT_DEVICE[] = "default";
  const std::size_t MINIMAL_LATENCY = 1000;

  inline void CheckAlsa(int res)
  {
    if (res < 0)
    {
      std::cout << ::snd_strerror(res);
      throw Error(ERROR_DETAIL, 1, ::snd_strerror(res));
    }
  }

  class AlsaBackend : public SimpleAsyncBackend
  {
  public:
    AlsaBackend() : DevHandle(0)
    {
    }

    virtual ~AlsaBackend()
    {
      assert(0 == DevHandle || "AlsaBackend was destroyed without stopping");
    }

    virtual void OnParametersChanged(unsigned changedFields)
    {
      const unsigned mask(DRIVER_PARAMS | DRIVER_FLAGS | BUFFER | SOUND_FREQ);
      if (changedFields & mask)
      {
        const bool needStartup(0 != DevHandle);
        OnShutdown();

        Parent::OnParametersChanged(changedFields);
        if (needStartup)
        {
          OnStartup();
        }
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
      Parent::OnStartup();
    }

    virtual void OnShutdown()
    {
      Parent::OnShutdown();
      if (0 != DevHandle)
      {
        CheckAlsa(::snd_pcm_close(DevHandle));
        DevHandle = 0;
      }
    }
  private:
    virtual void PlayBuffer(const Parent::Buffer& buf)
    {
      assert(0 != DevHandle);
      std::size_t toWrite(buf.Size / OUTPUT_CHANNELS);
      const Sample* data(&buf.Data[0]);
      while (toWrite)
      {
        snd_pcm_sframes_t res(::snd_pcm_writei(DevHandle, data, toWrite));
        if (res < 0)
        {
          if (!(res = ::snd_pcm_recover(DevHandle, res, 0)))
          {
            continue;
          }
        }
        CheckAlsa(res);
        toWrite -= res;
        data += res * OUTPUT_CHANNELS;
      }
    }
  private:
    snd_pcm_t* DevHandle;
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

