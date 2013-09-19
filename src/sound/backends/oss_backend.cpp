/*
Abstract:
  Oss backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "backend_impl.h"
#include "storage.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//platform-specific includes
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/soundcard.h>
//std includes
#include <algorithm>
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>
//text includes
#include "text/backends.h"

#define FILE_TAG 69200152

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Oss");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace Oss
{
  const String ID = Text::OSS_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("OSS sound system backend");
  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;

  const uint_t MAX_OSS_VOLUME = 100;

  class AutoDescriptor : public boost::noncopyable
  {
  public:
    AutoDescriptor()
      : Handle(-1)
    {
    }

    AutoDescriptor(const std::string& name, int mode)
      : Name(name)
      , Handle(::open(name.c_str(), mode, 0))
    {
      CheckResult(Valid(), THIS_LINE);
      Dbg("Opened device '%1%'", Name);
    }

    ~AutoDescriptor()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {
      }
    }
    
    bool Valid() const
    {
      return -1 != Handle;
    }

    void Swap(AutoDescriptor& rh)
    {
      std::swap(rh.Handle, Handle);
      std::swap(rh.Name, Name);
    }

    void Close()
    {
      if (Valid())
      {
        Dbg("Close device '%1%'", Name);
        int tmpHandle = -1;
        std::swap(Handle, tmpHandle);
        Name.clear();
        CheckResult(0 == ::close(tmpHandle), THIS_LINE);
      }
    }
    
    void Ioctl(int request, void* param, Error::LocationRef loc)
    {
      const int res = ::ioctl(Handle, request, param);
      CheckResult(res != -1, loc);
    }

    int WriteAsync(const void* data, std::size_t size)
    {
      for (;;)
      {
        const int res = ::write(Handle, data, size);
        if (-1 == res && errno == EAGAIN)
        {
          struct pollfd wait;
          wait.fd = Handle;
          wait.events = POLLOUT;
          const int pollerr = ::poll(&wait, 1, -1);
          CheckResult(pollerr > 0, THIS_LINE);
          continue;
        }
        CheckResult(res >= 0, THIS_LINE);
        return res;
      }
    }
  private:
    void CheckResult(bool res, Error::LocationRef loc) const
    {
      if (!res)
      {
        throw MakeFormattedError(loc,
          translate("Error in OSS backend while working with device '%1%': %2%."), Name, ::strerror(errno));
      }
    }
  private:
    std::string Name;
    //leave handle as int
    int Handle;
  };

  class SoundFormat
  {
  public:
    explicit SoundFormat(int supportedFormats)
      : Native(GetSoundFormat(Sample::MID == 0))
      , Negated(GetSoundFormat(Sample::MID != 0))
      , NativeSupported(0 != (supportedFormats & Native))
      , NegatedSupported(0 != (supportedFormats & Negated))
    {
    }
    
    bool IsSupported() const
    {
      return NativeSupported || NegatedSupported;
    }
    
    int Get() const
    {
      return NativeSupported
        ? Native
        : NegatedSupported ? Negated : -1;
    }
  private:
    static int GetSoundFormat(bool isSigned)
    {
      switch (sizeof(Sample))
      {
      case 1:
        return isSigned ? AFMT_S8 : AFMT_U8;
      case 2:
        return isSigned
          ? (isLE() ? AFMT_S16_LE : AFMT_S16_BE)
          : (isLE() ? AFMT_U16_LE : AFMT_U16_BE);
      default:
        assert(!"Invalid format");
        return -1;
      };
    }
  private:
    const int Native;
    const int Negated;
    const bool NativeSupported;
    const bool NegatedSupported;
  };


  class VolumeControl : public Sound::VolumeControl
  {
  public:
    VolumeControl(boost::mutex& stateMutex, AutoDescriptor& mixer)
      : StateMutex(stateMutex), MixHandle(mixer)
    {
    }

    virtual Gain GetVolume() const
    {
      Dbg("GetVolume");
      const boost::mutex::scoped_lock lock(StateMutex);
      Gain volume;
      if (MixHandle.Valid())
      {
        boost::array<uint8_t, sizeof(int)> buf;
        MixHandle.Ioctl(SOUND_MIXER_READ_VOLUME, &buf[0], THIS_LINE);
        volume = Gain(Gain::Type(buf[0], MAX_OSS_VOLUME), Gain::Type(buf[1], MAX_OSS_VOLUME));
      }
      return volume;
    }

    virtual void SetVolume(const Gain& volume)
    {
      if (!volume.IsNormalized())
      {
        throw Error(THIS_LINE, translate("Failed to set volume: gain is out of range."));
      }
      Dbg("SetVolume");
      const boost::mutex::scoped_lock lock(StateMutex);
      if (MixHandle.Valid())
      {
        boost::array<uint8_t, sizeof(int)> buf = { {0} };
        buf[0] = (volume.Left() * MAX_OSS_VOLUME).Integer();
        buf[1] = (volume.Right() * MAX_OSS_VOLUME).Integer();
        MixHandle.Ioctl(SOUND_MIXER_WRITE_VOLUME, &buf[0], THIS_LINE);
      }
    }
  private:
    boost::mutex& StateMutex;
    AutoDescriptor& MixHandle;
  };

  class BackendParameters
  {
  public:
    explicit BackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    String GetDeviceName() const
    {
      Parameters::StringType strVal = Parameters::ZXTune::Sound::Backends::Oss::DEVICE_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::Sound::Backends::Oss::DEVICE, strVal);
      return strVal;
    }

    String GetMixerName() const
    {
      Parameters::StringType strVal = Parameters::ZXTune::Sound::Backends::Oss::MIXER_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::Sound::Backends::Oss::MIXER, strVal);
      return strVal;
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    explicit BackendWorker(Parameters::Accessor::Ptr params)
      : Params(params)
      , Format(-1)
      , VolumeController(new VolumeControl(StateMutex, MixHandle))
    {
    }

    virtual ~BackendWorker()
    {
      assert(!DevHandle.Valid() || !"OssBackend should be stopped before destruction.");
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeController;
    }

    virtual void Startup()
    {
      assert(!MixHandle.Valid() && !DevHandle.Valid());
      SetupDevices(DevHandle, MixHandle, Format);
      Dbg("Successfully opened");
    }

    virtual void Shutdown()
    {
      DevHandle.Close();
      MixHandle.Close();
      Format = -1;
      Dbg("Successfully closed");
    }

    virtual void Pause()
    {
    }

    virtual void Resume()
    {
    }

    virtual void FrameStart(const Module::TrackState& /*state*/)
    {
    }

    virtual void FrameFinish(Chunk::Ptr buffer)
    {
      switch (Format)
      {
      case AFMT_S16_LE:
      case AFMT_S16_BE:
        buffer->ToS16();
        break;
      case AFMT_U8:
        buffer->ToU8();
        break;
      default:
        assert(!"Invalid format");
      }
      assert(DevHandle.Valid());
      std::size_t toWrite(buffer->size() * sizeof(buffer->front()));
      const uint8_t* data(safe_ptr_cast<const uint8_t*>(&(*buffer)[0]));
      while (toWrite)
      {
        const int res = DevHandle.WriteAsync(data, toWrite * sizeof(*data));
        toWrite -= res;
        data += res;
      }
    }
  private:
    void SetupDevices(AutoDescriptor& device, AutoDescriptor& mixer, int& fmt) const
    {
      const RenderParameters::Ptr sound = RenderParameters::Create(Params);
      const BackendParameters backend(*Params);

      AutoDescriptor tmpMixer(backend.GetMixerName(), O_RDWR);
      AutoDescriptor tmpDevice(backend.GetDeviceName(), O_WRONLY | O_NONBLOCK);
      BOOST_STATIC_ASSERT(8 == Sample::BITS || 16 == Sample::BITS);
      int tmp = 0;
      tmpDevice.Ioctl(SNDCTL_DSP_GETFMTS, &tmp, THIS_LINE);
      Dbg("Supported formats %1%", tmp);
      const SoundFormat format(tmp);
      if (!format.IsSupported())
      {
        throw Error(THIS_LINE, translate("No suitable formats supported by OSS."));
      }
      tmp = format.Get();
      Dbg("Setting format to %1%", tmp);
      tmpDevice.Ioctl(SNDCTL_DSP_SETFMT, &tmp, THIS_LINE);

      tmp = Sample::CHANNELS;
      Dbg("Setting channels to %1%", tmp);
      tmpDevice.Ioctl(SNDCTL_DSP_CHANNELS, &tmp, THIS_LINE);

      tmp = sound->SoundFreq();
      Dbg("Setting frequency to %1%", tmp);
      tmpDevice.Ioctl(SNDCTL_DSP_SPEED, &tmp, THIS_LINE);

      device.Swap(tmpDevice);
      mixer.Swap(tmpMixer);
      fmt = format.Get();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    boost::mutex StateMutex;
    AutoDescriptor MixHandle;
    AutoDescriptor DevHandle;
    int Format;
    const VolumeControl::Ptr VolumeController;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params) const
    {
      return boost::make_shared<BackendWorker>(params);
    }
  };
}//Oss
}//Sound

namespace Sound
{
  void RegisterOssBackend(BackendsStorage& storage)
  {
    const BackendWorkerFactory::Ptr factory = boost::make_shared<Oss::BackendWorkerFactory>();
    storage.Register(Oss::ID, Oss::DESCRIPTION, Oss::CAPABILITIES, factory);
  }
}
