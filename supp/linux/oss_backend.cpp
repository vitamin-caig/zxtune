#include "../sound_backend_impl.h"

#include <tools.h>
#include <error.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <boost/static_assert.hpp>

#include <cassert>

#define FILE_TAG 69200152

namespace
{
  using namespace ZXTune::Sound;

  const char DEVICE_NAME[] = "/dev/dsp";
  const char MIXER_NAME[] = "/dev/mixer";

  inline void CheckResult(bool res)
  {
    if (!res)
    {
      throw Error(ERROR_DETAIL, 1, ::strerror(errno));
    }
  }

  class OSSBackend : public BackendImpl
  {
  public:
    OSSBackend() : DevHandle(-1), MixHandle(-1)
    {
    }

    virtual ~OSSBackend()
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
      assert(-1 != DevHandle);
      const uint8_t* buff(static_cast<const uint8_t*>(data));
      while (sizeInBytes)
      {
        std::size_t toWrite(0);
        for (;;)
        {
          audio_buf_info info;
          CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_GETOSPACE, &info));
          toWrite = std::min<std::size_t>(sizeInBytes, info.fragments * info.fragsize);
          if (!toWrite || int(toWrite) > info.bytes)
          {
            ::usleep(1000);
          }
          else
          {
            break;
          }
        }
        CheckResult(-1 != ::write(DevHandle, buff, toWrite));
        buff += toWrite;
        sizeInBytes -= toWrite;
      }
    }

    virtual void OnParametersChanged(unsigned changedFields)
    {
      const unsigned mask(DRIVER_PARAMS | BUFFER | SOUND_FREQ);
      if (changedFields & mask)
      {
        const bool needStartup(-1 != DevHandle);
        OnShutdown();

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
      assert(-1 == DevHandle);
      DevHandle = ::open((DEVICE_NAME + Params.DriverParameters).c_str(), O_WRONLY | O_NONBLOCK, 0);
      CheckResult(-1 != DevHandle);
      MixHandle = ::open((MIXER_NAME + Params.DriverParameters).c_str(), O_WRONLY, 0);
      CheckResult(-1 != MixHandle);

      BOOST_STATIC_ASSERT(1 == sizeof(Sample) || 2 == sizeof(Sample));
      int tmp(2 == sizeof(Sample) ? AFMT_S16_NE : AFMT_S8);
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_SETFMT, &tmp));

      tmp = OUTPUT_CHANNELS;
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_CHANNELS, &tmp));

      tmp = Params.SoundParameters.SoundFreq;
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_SPEED, &tmp));

      SetVolume();
    }

    virtual void OnShutdown()
    {
      if (-1 != DevHandle)
      {
        CheckResult(0 == ::close(DevHandle));
        DevHandle = -1;
        assert(-1 != MixHandle);
        CheckResult(0 == ::close(MixHandle));
        MixHandle = -1;
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
      CheckResult(-1 != ::ioctl(MixHandle, SOUND_MIXER_WRITE_VOLUME, &vol));
    }
  private:
    int DevHandle;
    int MixHandle;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Backend::Ptr CreateOSSBackend()
    {
      return Backend::Ptr(new OSSBackend);
    }
  }
}
