/*
Abstract:
  OSS backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef OSS_SUPPORT

//local includes
#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"
//common includes
#include <tools.h>
#include <error_tools.h>
#include <logging.h>
//library includes
#include <io/fs_tools.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/sound_parameters.h>
//platform-specific includes
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/noncopyable.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 69200152

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::OSS");

  const uint_t MAX_OSS_VOLUME = 100;

  const Char OSS_BACKEND_ID[] = {'o', 's', 's', 0};
  const String OSS_BACKEND_VERSION(FromStdString("$Rev$"));

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
          Text::SOUND_ERROR_OSS_BACKEND_ERROR, Name, ::strerror(errno));
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

  class OSSVolumeControl : public VolumeControl
  {
  public:
    OSSVolumeControl(boost::mutex& stateMutex, AutoDescriptor& mixer)
      : StateMutex(stateMutex), MixHandle(mixer)
    {
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      try
      {
        Log::Debug(THIS_MODULE, "GetVolume");
        boost::mutex::scoped_lock lock(StateMutex);
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
        return Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_GAIN);
      }
      try
      {
        Log::Debug(THIS_MODULE, "SetVolume");
        boost::mutex::scoped_lock lock(StateMutex);
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
  private:
    boost::mutex& StateMutex;
    AutoDescriptor& MixHandle;
  };

  class OSSBackendParameters
  {
  public:
    explicit OSSBackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    String GetDeviceName() const
    {
      Parameters::StringType strVal = Parameters::ZXTune::Sound::Backends::OSS::DEVICE_DEFAULT;
      Accessor.FindStringValue(Parameters::ZXTune::Sound::Backends::OSS::DEVICE, strVal);
      return strVal;
    }

    String GetMixerName() const
    {
      Parameters::StringType strVal = Parameters::ZXTune::Sound::Backends::OSS::MIXER_DEFAULT;
      Accessor.FindStringValue(Parameters::ZXTune::Sound::Backends::OSS::MIXER, strVal);
      return strVal;
    }
  private:
    const Parameters::Accessor& Accessor;
  };


  class OSSBackend : public BackendImpl
                   , private boost::noncopyable
  {
  public:
    OSSBackend(BackendParameters::Ptr params, Module::Holder::Ptr module)
      : BackendImpl(params, module)
      , MixerName(Parameters::ZXTune::Sound::Backends::OSS::MIXER_DEFAULT)
      , DeviceName(Parameters::ZXTune::Sound::Backends::OSS::DEVICE_DEFAULT)
      , CurrentBuffer(Buffers.begin(), Buffers.end())
      , Samplerate(RenderingParameters->SoundFreq())
      , VolumeController(new OSSVolumeControl(StateMutex, MixHandle))
    {
      OnParametersChanged(*SoundParameters);
    }

    virtual ~OSSBackend()
    {
      assert(-1 == DevHandle.Get() || !"OSSBackend should be stopped before destruction.");
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeController;
    }

    virtual void OnStartup()
    {
      DoStartup();
    }

    virtual void OnShutdown()
    {
      DoShutdown();
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    void OnParametersChanged(const Parameters::Accessor& updates)
    {
      const OSSBackendParameters curParams(updates);

      //check for parameters requires restarting
      const String& newDevice = curParams.GetDeviceName();
      const String& newMixer = curParams.GetMixerName();
      const uint_t newFreq = RenderingParameters->SoundFreq();

      const bool deviceChanged = newDevice != DeviceName;
      const bool mixerChanged = newMixer != MixerName;
      const bool freqChanged = newFreq != Samplerate;
      if (deviceChanged || mixerChanged || freqChanged)
      {
        boost::mutex::scoped_lock lock(StateMutex);
        const bool needStartup(-1 != DevHandle.Get());
        DoShutdown();
        Log::Debug(THIS_MODULE, "Device %1% => %2%", DeviceName, newDevice);
        DeviceName = newDevice;
        Log::Debug(THIS_MODULE, "Mixer %1% => %2%", MixerName, newMixer);
        MixerName = newMixer;
        Log::Debug(THIS_MODULE, "Samplerate %1% => %2%", Samplerate, newFreq);
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
      Log::Debug(THIS_MODULE, "Setting format to %1%", tmp);
      tmpDevice.CheckResult(-1 != ::ioctl(tmpDevice.Get(), SNDCTL_DSP_SETFMT, &tmp), THIS_LINE);

      tmp = OUTPUT_CHANNELS;
      Log::Debug(THIS_MODULE, "Setting channels to %1%", tmp);
      tmpDevice.CheckResult(-1 != ::ioctl(tmpDevice.Get(), SNDCTL_DSP_CHANNELS, &tmp), THIS_LINE);

      tmp = Samplerate;
      Log::Debug(THIS_MODULE, "Setting frequency to %1%", tmp);
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
    boost::mutex StateMutex;
    String MixerName;
    AutoDescriptor MixHandle;
    String DeviceName;
    AutoDescriptor DevHandle;
    boost::array<std::vector<MultiSample>, 2> Buffers;
    CycledIterator<std::vector<MultiSample>*> CurrentBuffer;
    uint_t Samplerate;
    VolumeControl::Ptr VolumeController;
  };

  class OSSBackendCreator : public BackendCreator
  {
  public:
    virtual String Id() const
    {
      return OSS_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::OSS_BACKEND_DESCRIPTION;
    }

    virtual String Version() const
    {
      return OSS_BACKEND_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;
    }

    virtual Error CreateBackend(BackendParameters::Ptr params, Module::Holder::Ptr module, Backend::Ptr& result) const
    {
      return SafeBackendWrapper<OSSBackend>::Create(Id(), params, module, result, THIS_LINE);
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterOSSBackend(BackendsEnumerator& enumerator)
    {
      const BackendCreator::Ptr creator(new OSSBackendCreator());
      enumerator.RegisterCreator(creator);
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
