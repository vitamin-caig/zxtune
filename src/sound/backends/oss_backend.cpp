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
#include <logging.h>
#include <io/fs_tools.h>
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
  
  const std::string THIS_MODULE("OSSBackend");

  const uint_t MAX_OSS_VOLUME = 100;
  
  const Char BACKEND_ID[] = {'o', 's', 's', 0};
  const String BACKEND_VERSION(FromStdString("$Rev$"));

  static const BackendInformation BACKEND_INFO =
  {
    BACKEND_ID,
    TEXT_OSS_BACKEND_DESCRIPTION,
    BACKEND_VERSION,
  };
 
  class AutoDescriptor : public boost::noncopyable
  {
  public:
    AutoDescriptor()
      : Handle(-1)
    {
    }
    AutoDescriptor(const String& name, int mode)
      : Name(name)
      , Handle(::open(IO::ConvertToFilename(name).c_str(), mode, 0))
    {
      CheckResult(-1 != Handle, THIS_LINE);
    }
    
    ~AutoDescriptor()
    {
      if (-1 != Handle)
      {
        ::close(Handle);
      }
    }
    
    void CheckResult(bool res, Error::LocationRef loc) const
    {
      if (!res)
      {
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
          TEXT_SOUND_ERROR_OSS_BACKEND_ERROR, Name, ::strerror(errno));
      }
    }

    void Swap(AutoDescriptor& rh)
    {
      std::swap(rh.Handle, Handle);
      std::swap(rh.Name, Name);
    }
    
    void Close()
    {
      if (-1 != Handle)
      {
        CheckResult(0 == ::close(Handle), THIS_LINE);
        Handle = -1;
        Name.clear();
      }
    }
    
    int Get() const
    {
      return Handle;
    }
  private:
    String Name;
    //leave handle as int
    int Handle;
  };
  
  class OSSBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    OSSBackend()
      : MixerName(Parameters::ZXTune::Sound::Backends::OSS::MIXER_DEFAULT)
      , DeviceName(Parameters::ZXTune::Sound::Backends::OSS::DEVICE_DEFAULT)
      , CurrentBuffer(Buffers.begin(), Buffers.end())
    {
    }

    virtual ~OSSBackend()
    {
      assert(-1 == DevHandle.Get() || !"OSSBackend should be stopped before destruction.");
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
        if (-1 != MixHandle.Get())
        {
          boost::array<uint8_t, sizeof(int)> buf;
          MixHandle.CheckResult(-1 != ::ioctl(MixHandle.Get(), SOUND_MIXER_READ_VOLUME, 
            safe_ptr_cast<int*>(&buf[0])), THIS_LINE);
          std::transform(buf.begin(), buf.begin() + OUTPUT_CHANNELS, volume.begin(),
            std::bind2nd(std::divides<Gain>(), MAX_OSS_VOLUME));
        }
        else
        {
          volume = MultiGain();
        }
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
        if (-1 != MixHandle.Get())
        {
          boost::array<uint8_t, sizeof(int)> buf = { {0} };
          std::transform(volume.begin(), volume.end(), buf.begin(),
            std::bind2nd(std::multiplies<Gain>(), MAX_OSS_VOLUME));
          MixHandle.CheckResult(-1 != ::ioctl(MixHandle.Get(), SOUND_MIXER_WRITE_VOLUME, 
            safe_ptr_cast<int*>(&buf[0])), THIS_LINE);
        }
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
        const bool needStartup(-1 != DevHandle.Get());
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
      assert(-1 != DevHandle.Get());
      std::size_t toWrite(buf.size() * sizeof(buf.front()));
      const uint8_t* data(safe_ptr_cast<const uint8_t*>(&buf[0]));
      while (toWrite)
      {
        const int res = ::write(DevHandle.Get(), data, toWrite * sizeof(*data));
        DevHandle.CheckResult(res >= 0, THIS_LINE);
        toWrite -= res;
        data += res;
      }
      ++CurrentBuffer;
    }
  private:
    void DoStartup()
    {
      Log::Debug(THIS_MODULE, "Opening mixer='%1%' and device='%2%'", MixerName, DeviceName);
      assert(-1 == MixHandle.Get() && -1 == DevHandle.Get());
      
      AutoDescriptor tmpMixer(MixerName, O_RDWR);
      AutoDescriptor tmpDevice(DeviceName, O_WRONLY);
      
      BOOST_STATIC_ASSERT(1 == sizeof(Sample) || 2 == sizeof(Sample));
      int tmp(2 == sizeof(Sample) ? AFMT_S16_NE : AFMT_S8);
      tmpDevice.CheckResult(-1 != ::ioctl(tmpDevice.Get(), SNDCTL_DSP_SETFMT, &tmp), THIS_LINE);

      tmp = OUTPUT_CHANNELS;
      tmpDevice.CheckResult(-1 != ::ioctl(tmpDevice.Get(), SNDCTL_DSP_CHANNELS, &tmp), THIS_LINE);

      tmp = RenderingParameters.SoundFreq;
      tmpDevice.CheckResult(-1 != ::ioctl(tmpDevice.Get(), SNDCTL_DSP_SPEED, &tmp), THIS_LINE);
      
      DevHandle.Swap(tmpDevice);
      MixHandle.Swap(tmpMixer);
      Log::Debug(THIS_MODULE, "Successfully opened");
    }

    void DoShutdown()
    {
      Log::Debug(THIS_MODULE, "Closing all the devices");
      DevHandle.Close();
      MixHandle.Close();
      Log::Debug(THIS_MODULE, "Successfully closed");
    }
  private:
    String MixerName;
    AutoDescriptor MixHandle;
    String DeviceName;
    AutoDescriptor DevHandle;
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
