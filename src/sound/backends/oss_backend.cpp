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
#include <error_tools.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>

//platform-specific
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <boost/noncopyable.hpp>

#include <algorithm>

#include <text/backends.h>
#include <text/sound.h>

#define FILE_TAG 69200152

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const unsigned MAX_OSS_VOLUME = 100;
  
  const Char BACKEND_ID[] = {'o', 's', 's', 0};
  const String BACKEND_VERSION(FromChar("$Rev$"));

  static const BackendInformation BACKEND_INFO =
  {
    BACKEND_ID,
    TEXT_OSS_BACKEND_DESCRIPTION,
    BACKEND_VERSION,
  };
  
  inline void CheckResult(bool res, Error::LocationRef loc)
  {
    if (!res)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
        TEXT_SOUND_ERROR_OSS_BACKEND_ERROR, ::strerror(errno));
    }
  }
  
  class OSSBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    OSSBackend()
      : MixerName(Parameters::ZXTune::Sound::Backends::OSS::MIXER_DEFAULT)
      , MixHandle(-1)
      , DeviceName(Parameters::ZXTune::Sound::Backends::OSS::DEVICE_DEFAULT)
      , DevHandle(-1)
      , CurrentBuffer(Buffers.begin(), Buffers.end())
    {
    }

    virtual ~OSSBackend()
    {
      assert((-1 == DevHandle && -1 == MixHandle) || "OSSBackend was destroyed without stopping");
    }

    virtual void GetInformation(BackendInformation& info) const
    {
      info = BACKEND_INFO;
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      try
      {
        Locker lock(BackendMutex);
        assert(-1 != MixHandle);
        boost::array<uint8_t, sizeof(int)> buf;
        CheckResult(-1 != ::ioctl(MixHandle, SOUND_MIXER_READ_VOLUME, safe_ptr_cast<int*>(&buf[0])), THIS_LINE);
        std::transform(buf.begin(), buf.begin() + OUTPUT_CHANNELS, volume.begin(),
          std::bind2nd(std::divides<Gain>(), MAX_OSS_VOLUME));
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    virtual Error SetVolume(const MultiGain& volume)
    {
      if (volume.end() != std::find_if(volume.begin(), volume.end(), std::bind2nd(std::greater<Gain>(), Gain(1.0))))
      {
        return Error(THIS_LINE, BACKEND_INVALID_PARAMETER, TEXT_SOUND_ERROR_BACKEND_INVALID_GAIN);
      }
      try
      {
        Locker lock(BackendMutex);
        assert(-1 != MixHandle);
        boost::array<uint8_t, sizeof(int)> buf = { {0} };
        std::transform(volume.begin(), volume.end(), buf.begin(),
          std::bind2nd(std::multiplies<Gain>(), MAX_OSS_VOLUME));
        CheckResult(-1 != ::ioctl(MixHandle, SOUND_MIXER_WRITE_VOLUME, safe_ptr_cast<int*>(&buf[0])), THIS_LINE);
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

    virtual void OnStartup()
    {
      Locker lock(BackendMutex);
      DoStartup();
    }

    virtual void OnShutdown()
    {
      Locker lock(BackendMutex);
      DoShutdown();
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnParametersChanged(const Parameters::Map& updates)
    {
      //check for parameters requires restarting
      const Parameters::StringType* const mixer = 
        Parameters::FindByName<Parameters::StringType>(updates, Parameters::ZXTune::Sound::Backends::OSS::MIXER);
      const Parameters::StringType* const device = 
        Parameters::FindByName<Parameters::StringType>(updates, Parameters::ZXTune::Sound::Backends::OSS::DEVICE);
      const Parameters::IntType* const freq = 
        Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::FREQUENCY);
      if (mixer || device || freq)
      {
        Locker lock(BackendMutex);
        const bool needStartup(-1 != DevHandle);
        DoShutdown();
        if (mixer)
        {
          MixerName = *mixer;
        }
        if (device)
        {
          DeviceName = *device;
        }
        if (needStartup)
        {
          DoStartup();
        }
      }
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      Locker lock(BackendMutex);
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
    void DoStartup()
    {
      assert(-1 == MixHandle);
      MixHandle = ::open(MixerName.c_str(), O_RDWR, 0);
      CheckResult(-1 != MixHandle, THIS_LINE);

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

    void DoShutdown()
    {
      if (-1 != DevHandle)
      {
        CheckResult(0 == ::close(DevHandle), THIS_LINE);
        DevHandle = -1;
      }
      if (-1 == MixHandle)
      {
        CheckResult(0 == ::close(MixHandle), THIS_LINE);
        MixHandle = -1;
      }
    }
  private:
    String MixerName;
    int MixHandle;
    String DeviceName;
    int DevHandle;
    boost::array<std::vector<MultiSample>, 2> Buffers;
    cycled_iterator<std::vector<MultiSample>*> CurrentBuffer;
  };
  
  Backend::Ptr OSSBackendCreator(const Parameters::Map& params)
  {
    return Backend::Ptr(new SafeBackendWrapper<OSSBackend>(params));
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterOSSBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterBackend(BACKEND_INFO, OSSBackendCreator, BACKEND_PRIORITY_HIGH);
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
