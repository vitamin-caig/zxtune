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

#include <tools.h>
#include <sound/error_codes.h>
#include <sound/backends_parameters.h>

//platform-specific
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <boost/noncopyable.hpp>

#include <algorithm>

#include <text/sound.h>
#include <text/backends.h>

#define FILE_TAG 69200152

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const unsigned MAX_OSS_VOLUME = 100;
  
  const Char BACKEND_ID[] = {'o', 's', 's', 0};
  const String BACKEND_VERSION(FromChar("$Rev$"));

  static const Backend::Info BACKEND_INFO =
  {
    BACKEND_ID,
    BACKEND_VERSION,
    TEXT_OSS_BACKEND_DESCRIPTION
  };
  
  inline void CheckResult(bool res, Error::LocationRef loc)
  {
    if (!res)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, 
        TEXT_SOUND_ERROR_OSS_BACKEND_ERROR, ::strerror(errno));
    }
  }
  
  inline Error GetResult(bool res, Error::LocationRef loc)
  {
    return res ? 
      Error()
      : 
      MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, 
        TEXT_SOUND_ERROR_OSS_BACKEND_ERROR, ::strerror(errno));
  }
  
  class OSSBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    OSSBackend()
      : MixerName(Parameters::Sound::Backends::OSS::MIXER_DEFAULT)
      , MixHandle(-1)
      , DeviceName(Parameters::Sound::Backends::OSS::DEVICE_DEFAULT)
      , DevHandle(-1)
      , CurrentBuffer(Buffers.begin(), Buffers.end())
    {
      MixHandle = ::open(MixerName.c_str(), O_RDWR, 0);
      CheckResult(-1 != MixHandle, THIS_LINE);
    }

    virtual ~OSSBackend()
    {
      ::close(MixHandle);
      assert(-1 == DevHandle || "OSSBackend was destroyed without stopping");
    }

    virtual void GetInfo(Backend::Info& info) const
    {
      info = BACKEND_INFO;
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      Locker lock(BackendMutex);
      assert(-1 != MixHandle);
      boost::array<uint8_t, sizeof(int)> buf;
      if (-1 == ::ioctl(MixHandle, SOUND_MIXER_READ_VOLUME, safe_ptr_cast<int*>(&buf[0])))
      {
        return GetResult(false, THIS_LINE);
      }
      std::transform(buf.begin(), buf.begin() + OUTPUT_CHANNELS, volume.begin(), 
        std::bind2nd(std::divides<Gain>(), MAX_OSS_VOLUME));
      return Error();
    }

    virtual Error SetVolume(const MultiGain& volume)
    {
      Locker lock(BackendMutex);
      if (volume.end() != std::find_if(volume.begin(), volume.end(), std::bind2nd(std::greater<Gain>(), Gain(1.0))))
      {
        return Error(THIS_LINE, BACKEND_INVALID_PARAMETER, TEXT_SOUND_ERROR_BACKEND_INVALID_GAIN);
      }
      assert(-1 != MixHandle);
      boost::array<uint8_t, sizeof(int)> buf = { {0} };
      std::transform(volume.begin(), volume.end(), buf.begin(), 
        std::bind2nd(std::multiplies<Gain>(), MAX_OSS_VOLUME));
      return GetResult(-1 != ::ioctl(MixHandle, SOUND_MIXER_WRITE_VOLUME, safe_ptr_cast<int*>(&buf[0])), THIS_LINE);
    }

    virtual void OnStartup()
    {
      Locker lock(BackendMutex);
      assert(-1 == DevHandle);
      DevHandle = ::open(DeviceName.c_str(), O_WRONLY, 0);
      CheckResult(-1 != DevHandle, THIS_LINE);

      BOOST_STATIC_ASSERT(1 == sizeof(Sample) || 2 == sizeof(Sample));
      int tmp(2 == sizeof(Sample) ? AFMT_S16_NE : AFMT_S8);
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_SETFMT, &tmp), THIS_LINE);

      tmp = OUTPUT_CHANNELS;
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_CHANNELS, &tmp), THIS_LINE);

      tmp = RenderingParameters.SoundFreq;
      CheckResult(-1 != ::ioctl(DevHandle, SNDCTL_DSP_SPEED, &tmp), THIS_LINE);
    }

    virtual void OnShutdown()
    {
      Locker lock(BackendMutex);
      if (-1 != DevHandle)
      {
        CheckResult(0 == ::close(DevHandle), THIS_LINE);
        DevHandle = -1;
      }
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnParametersChanged(const ParametersMap& updates)
    {
      //process mutex
      {
        Locker lock(BackendMutex);
        //update mixer
        if (const String* mixer = FindParameter<String>(updates, Parameters::Sound::Backends::OSS::MIXER))
        {
          MixerName = *mixer;
          CheckResult(0 == ::close(MixHandle), THIS_LINE);
          MixHandle = ::open(MixerName.c_str(), O_RDWR, 0);
          CheckResult(-1 != MixHandle, THIS_LINE);
        }
      }
      //check for parameters requires restarting
      const String* const device = FindParameter<String>(updates, Parameters::Sound::Backends::OSS::DEVICE);
      const int64_t* const freq = FindParameter<int64_t>(updates, Parameters::Sound::FREQUENCY);
      if (device || freq)
      {
        DeviceName = *device;
        const bool needStartup(-1 != DevHandle);
        OnShutdown();
        if (needStartup)
        {
          OnStartup();
        }
      }
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
        CheckResult(res >= 0, THIS_LINE);
        toWrite -= res;
        data += res;
      }
      ++CurrentBuffer;
    }

  private:
    String MixerName;
    int MixHandle;
    String DeviceName;
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
