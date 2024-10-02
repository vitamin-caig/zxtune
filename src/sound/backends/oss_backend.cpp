/**
 *
 * @file
 *
 * @brief  OSS backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/backend_impl.h"
#include "sound/backends/l10n.h"
#include "sound/backends/oss.h"
#include "sound/backends/storage.h"
// common includes
#include <byteorder.h>
#include <error_tools.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <debug/log.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
// platform-specific includes
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <unistd.h>
// std includes
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <mutex>

namespace Sound::Oss
{
  const Debug::Stream Dbg("Sound::Backend::Oss");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;

  const int_t MAX_OSS_VOLUME = 100;

  class AutoDescriptor
  {
  public:
    AutoDescriptor() = default;

    explicit AutoDescriptor(StringView name)
      : Name(name)
    {}

    AutoDescriptor(StringView name, int mode)
      : Name(name)
      , Handle(::open(Name.c_str(), mode, 0))
    {
      CheckResult(Valid(), THIS_LINE);
      Dbg("Opened device '{}'", Name);
    }

    AutoDescriptor(const AutoDescriptor&) = delete;

    ~AutoDescriptor()
    {
      try
      {
        Close();
      }
      catch (const Error&)
      {}
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
        Dbg("Close device '{}'", Name);
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

    void CheckStat() const
    {
      struct stat sb;
      CheckResult(0 == ::stat(Name.c_str(), &sb), THIS_LINE);
    }

  private:
    void CheckResult(bool res, Error::LocationRef loc) const
    {
      if (!res)
      {
        throw MakeFormattedError(loc, translate("Error in OSS backend while working with device '{0}': {1}."), Name,
                                 ::strerror(errno));
      }
    }

  private:
    String Name;
    // leave handle as int
    int Handle = -1;
  };

  class SoundFormat
  {
  public:
    explicit SoundFormat(int supportedFormats)
      : Native(GetSoundFormat(Sample::MID == 0))
      , Negated(GetSoundFormat(Sample::MID != 0))
      , NativeSupported(0 != (supportedFormats & Native))
      , NegatedSupported(0 != (supportedFormats & Negated))
    {}

    bool IsSupported() const
    {
      return NativeSupported || NegatedSupported;
    }

    int Get() const
    {
      return NativeSupported ? Native : NegatedSupported ? Negated : -1;
    }

  private:
    static int GetSoundFormat(bool isSigned)
    {
      switch (Sample::BITS)
      {
      case 8:
        return isSigned ? AFMT_S8 : AFMT_U8;
      case 16:
        return isSigned ? (isLE() ? AFMT_S16_LE : AFMT_S16_BE) : (isLE() ? AFMT_U16_LE : AFMT_U16_BE);
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
    VolumeControl(std::mutex& stateMutex, AutoDescriptor& mixer)
      : StateMutex(stateMutex)
      , MixHandle(mixer)
    {}

    Gain GetVolume() const override
    {
      Dbg("GetVolume");
      const std::lock_guard<std::mutex> lock(StateMutex);
      Gain volume;
      if (MixHandle.Valid())
      {
        std::array<uint8_t, sizeof(int)> buf;
        MixHandle.Ioctl(SOUND_MIXER_READ_VOLUME, buf.data(), THIS_LINE);
        volume = Gain(Gain::Type(buf[0], MAX_OSS_VOLUME), Gain::Type(buf[1], MAX_OSS_VOLUME));
      }
      return volume;
    }

    void SetVolume(const Gain& volume) override
    {
      if (!volume.IsNormalized())
      {
        throw Error(THIS_LINE, translate("Failed to set volume: gain is out of range."));
      }
      Dbg("SetVolume");
      const std::lock_guard<std::mutex> lock(StateMutex);
      if (MixHandle.Valid())
      {
        std::array<uint8_t, sizeof(int)> buf = {{0}};
        buf[0] = (volume.Left() * MAX_OSS_VOLUME).Integer();
        buf[1] = (volume.Right() * MAX_OSS_VOLUME).Integer();
        MixHandle.Ioctl(SOUND_MIXER_WRITE_VOLUME, buf.data(), THIS_LINE);
      }
    }

  private:
    std::mutex& StateMutex;
    AutoDescriptor& MixHandle;
  };

  class BackendParameters
  {
  public:
    explicit BackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {}

    String GetDeviceName() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Oss;
      return Parameters::GetString(Accessor, DEVICE, DEVICE_DEFAULT);
    }

    String GetMixerName() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Oss;
      return Parameters::GetString(Accessor, MIXER, MIXER_DEFAULT);
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    explicit BackendWorker(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
      , VolumeController(new VolumeControl(StateMutex, MixHandle))
    {}

    ~BackendWorker() override
    {
      assert(!DevHandle.Valid() || !"OssBackend should be stopped before destruction.");
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return VolumeController;
    }

    void Startup() override
    {
      assert(!MixHandle.Valid() && !DevHandle.Valid());
      SetupDevices(DevHandle, MixHandle, Format);
      Dbg("Successfully opened");
    }

    void Shutdown() override
    {
      DevHandle.Close();
      MixHandle.Close();
      Format = -1;
      Dbg("Successfully closed");
    }

    void Pause() override {}

    void Resume() override {}

    void FrameStart(const Module::State& /*state*/) override {}

    void FrameFinish(Chunk buffer) override
    {
      switch (Format)
      {
      case AFMT_S16_LE:
      case AFMT_S16_BE:
        buffer.ToS16();
        break;
      case AFMT_U8:
        buffer.ToU8();
        break;
      default:
        assert(!"Invalid format");
      }
      assert(DevHandle.Valid());
      std::size_t toWrite(buffer.size() * sizeof(buffer.front()));
      const auto* data(safe_ptr_cast<const uint8_t*>(buffer.data()));
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
      static_assert(8 == Sample::BITS || 16 == Sample::BITS, "Incompatible sound sample bits count");
      int tmp = 0;
      tmpDevice.Ioctl(SNDCTL_DSP_GETFMTS, &tmp, THIS_LINE);
      Dbg("Supported formats {}", tmp);
      const SoundFormat format(tmp);
      if (!format.IsSupported())
      {
        throw Error(THIS_LINE, translate("No suitable formats supported by OSS."));
      }
      tmp = format.Get();
      Dbg("Setting format to {}", tmp);
      tmpDevice.Ioctl(SNDCTL_DSP_SETFMT, &tmp, THIS_LINE);

      tmp = Sample::CHANNELS;
      Dbg("Setting channels to {}", tmp);
      tmpDevice.Ioctl(SNDCTL_DSP_CHANNELS, &tmp, THIS_LINE);

      tmp = sound->SoundFreq();
      Dbg("Setting frequency to {}", tmp);
      tmpDevice.Ioctl(SNDCTL_DSP_SPEED, &tmp, THIS_LINE);

      device.Swap(tmpDevice);
      mixer.Swap(tmpMixer);
      fmt = format.Get();
    }

  private:
    const Parameters::Accessor::Ptr Params;
    std::mutex StateMutex;
    AutoDescriptor MixHandle;
    AutoDescriptor DevHandle;
    int Format = -1;
    const VolumeControl::Ptr VolumeController;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr /*holder*/) const override
    {
      const BackendParameters backend(*params);
      AutoDescriptor(backend.GetMixerName()).CheckStat();
      AutoDescriptor(backend.GetDeviceName()).CheckStat();
      return MakePtr<BackendWorker>(params);
    }
  };
}  // namespace Sound::Oss

namespace Sound
{
  void RegisterOssBackend(BackendsStorage& storage)
  {
    auto factory = MakePtr<Oss::BackendWorkerFactory>();
    storage.Register(Oss::BACKEND_ID, Oss::BACKEND_DESCRIPTION, Oss::CAPABILITIES, std::move(factory));
  }
}  // namespace Sound
