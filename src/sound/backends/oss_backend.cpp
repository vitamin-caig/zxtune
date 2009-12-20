/*
Abstract:
  OSS backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef OSS_SUPPORT

#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"
#include "../error_codes.h"

#include <tools.h>

//platform-specific
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <boost/noncopyable.hpp>

#include <algorithm>

#include <text/backends.h>

#define FILE_TAG 69200152

namespace
{
  using namespace ZXTune::Sound;

  const Char BACKEND_ID[] = {'o', 's', 's', 0};
  const String BACKEND_VERSION(FromChar("$Rev$"));

  static const Backend::Info BACKEND_INFO =
  {
    BACKEND_ID,
    BACKEND_VERSION,
    TEXT_OSS_BACKEND_DESCRIPTION
  };

  const char DEVICE_NAME[] = "/dev/dsp";

  inline void CheckResult(bool res)
  {
    if (!res)
    {
      throw MakeFormattedError(THIS_LINE, BACKEND_PLATFORM_ERROR, 
                               TEXT_SOUND_ERROR_OSS_BACKEND_ERROR, ::strerror(errno));
    }
  }
  
  class OSSBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    OSSBackend()
      : DeviceName(DEVICE_NAME)
      , DevHandle(-1)
      , CurrentBuffer(Buffers.begin(), Buffers.end())
    {
    }

    virtual ~OSSBackend()
    {
      assert(-1 == DevHandle || "OSSBackend was destroyed without stopping");
    }

    virtual void GetInfo(Backend::Info& info) const
    {
      info = BACKEND_INFO;
    }

    virtual Error GetVolume(MultiGain& /*volume*/) const
    {
      return Error();//TODO
    }

    virtual Error SetVolume(const MultiGain& /*volume*/)
    {
      return Error();//TODO
    }

    virtual void OnStartup()
    {
      assert(-1 == DevHandle);
      DevHandle = ::open(DeviceName.c_str(), O_WRONLY, 0);
      CheckResult(-1 != DevHandle);

      BOOST_STATIC_ASSERT(1 == sizeof(Sample) || 2 == sizeof(Sample));
      int tmp(2 == sizeof(Sample) ? AFMT_S16_NE : AFMT_S8);
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_SETFMT, &tmp));

      tmp = OUTPUT_CHANNELS;
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_CHANNELS, &tmp));

      tmp = RenderingParameters.SoundFreq;
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_SPEED, &tmp));
    }

    virtual void OnShutdown()
    {
      if (-1 != DevHandle)
      {
        CheckResult(0 == ::close(DevHandle));
        DevHandle = -1;
      }
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnParametersChanged(const ParametersMap& /*updates*/)
    {
      /*TODO
      const unsigned mask(DRIVER_PARAMS | BUFFER | SOUND_FREQ);
      if (changedFields & mask)
      {
        const bool needStartup(-1 != DevHandle);
        OnShutdown();

        Parent::OnParametersChanged(changedFields);
        if (needStartup)
        {
          OnStartup();
        }
      }
      */
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      std::vector<MultiSample>& buf(*CurrentBuffer);
      buf.swap(buffer);
      assert(-1 != DevHandle);
      std::size_t toWrite(buf.size() * sizeof(buf.front()));
      const uint8_t* data(safe_ptr_cast<const uint8_t*>(&buf[0]));
      while (toWrite)
      {
        int res(::write(DevHandle, data, toWrite * sizeof(*data)));
        CheckResult(res >= 0);
        toWrite -= res;
        data += res;
      }
      ++CurrentBuffer;
    }

  private:
    std::string DeviceName;
    int DevHandle;
    boost::array<std::vector<MultiSample>, 2> Buffers;
    cycled_iterator<std::vector<MultiSample>*> CurrentBuffer;
  };
  
  Backend::Ptr OSSBackendCreator()
  {
    return Backend::Ptr(new SafeBackendWrapper<OSSBackend>());
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterOSSBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterBackend(BACKEND_INFO, OSSBackendCreator);
    }
  }
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterOSSBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}

#endif
