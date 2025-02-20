/**
 *
 * @file
 *
 * @brief  Win32 backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/backend_impl.h"
#include "sound/backends/gates/win32_api.h"
#include "sound/backends/l10n.h"
#include "sound/backends/storage.h"
#include "sound/backends/volume_control.h"
#include "sound/backends/win32.h"

#include "debug/log.h"
#include "math/numeric.h"
#include "sound/backends_parameters.h"
#include "sound/render_params.h"
#include "sound/sound_parameters.h"
#include "strings/encoding.h"

#include "contract.h"
#include "error_tools.h"
#include "make_ptr.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace Sound::Win32
{
  const Debug::Stream Dbg("Sound::Backend::Win32");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;

  const int_t MAX_WIN32_VOLUME = 0xffff;
  const uint_t BUFFERS_MIN = 3;
  const uint_t BUFFERS_MAX = 10;

  class SharedEvent
  {
  public:
    SharedEvent()
      : Handle(::CreateEvent(0, FALSE, FALSE, 0), &::CloseHandle)
    {}

    void Wait() const
    {
      CheckPlatformResult(WAIT_OBJECT_0 == ::WaitForSingleObject(Get(), INFINITE), THIS_LINE);
    }

    ::HANDLE Get() const
    {
      return Handle.get();
    }

  private:
    static void CheckPlatformResult(bool val, Error::LocationRef loc)
    {
      if (!val)
      {
        // TODO: convert code to string
        throw MakeFormattedError(loc, translate("Error in Win32 backend: code {}."), ::GetLastError());
      }
    }

  private:
    const std::shared_ptr<void> Handle;
  };

  // lightweight wrapper around HWAVEOUT handle
  class WaveOutDevice
  {
  public:
    using Ptr = std::shared_ptr<WaveOutDevice>;

    WaveOutDevice(Api::Ptr api, const ::WAVEFORMATEX& format, UINT device)
      : WinApi(std::move(api))
    {
      Dbg("Opening device {} ({} Hz)", device, format.nSamplesPerSec);
      CheckMMResult(
          WinApi->waveOutOpen(&Handle, device, &format, DWORD_PTR(Event.Get()), 0, CALLBACK_EVENT | WAVE_FORMAT_DIRECT),
          THIS_LINE);
    }

    ~WaveOutDevice()
    {
      try
      {
        Close();
      }
      catch (const Error& e)
      {
        Dbg("Failed to close device: {}", e.ToString());
      }
    }

    void Close()
    {
      if (Handle)
      {
        Dbg("Closing device");
        CheckMMResult(WinApi->waveOutReset(Handle), THIS_LINE);
        CheckMMResult(WinApi->waveOutClose(Handle), THIS_LINE);
        Handle = nullptr;
      }
    }

    void WaitForBufferComplete()
    {
      Event.Wait();
    }

    ::HWAVEOUT Get() const
    {
      return Handle;
    }

    void PrepareHeader(::WAVEHDR& header)
    {
      CheckMMResult(WinApi->waveOutPrepareHeader(Handle, &header, sizeof(header)), THIS_LINE);
    }

    void UnprepareHeader(::WAVEHDR& header)
    {
      CheckMMResult(WinApi->waveOutUnprepareHeader(Handle, &header, sizeof(header)), THIS_LINE);
    }

    void Write(::WAVEHDR& header)
    {
      CheckMMResult(WinApi->waveOutWrite(Handle, &header, sizeof(header)), THIS_LINE);
    }

    void Pause()
    {
      CheckMMResult(WinApi->waveOutPause(Handle), THIS_LINE);
    }

    void Resume()
    {
      CheckMMResult(WinApi->waveOutRestart(Handle), THIS_LINE);
    }

    void GetVolume(LPDWORD val)
    {
      CheckMMResult(WinApi->waveOutGetVolume(Handle, val), THIS_LINE);
    }

    void SetVolume(DWORD val)
    {
      CheckMMResult(WinApi->waveOutSetVolume(Handle, val), THIS_LINE);
    }

  private:
    void CheckMMResult(::MMRESULT res, Error::LocationRef loc) const
    {
      if (MMSYSERR_NOERROR != res)
      {
        std::vector<char> buffer(1024);
        if (MMSYSERR_NOERROR == WinApi->waveOutGetErrorTextA(res, buffer.data(), static_cast<UINT>(buffer.size())))
        {
          throw MakeFormattedError(loc, translate("Error in Win32 backend: {}."),
                                   String(buffer.begin(), std::find(buffer.begin(), buffer.end(), '\0')));
        }
        else
        {
          throw MakeFormattedError(loc, translate("Error in Win32 backend: code {}."), res);
        }
      }
    }

  private:
    const Api::Ptr WinApi;
    const SharedEvent Event;
    ::HWAVEOUT Handle = nullptr;
  };

  class WaveTarget
  {
  public:
    using Ptr = std::shared_ptr<WaveTarget>;
    virtual ~WaveTarget() = default;

    virtual void Write(const Chunk& buf) = 0;
  };

  class WaveBuffer : public WaveTarget
  {
  public:
    explicit WaveBuffer(WaveOutDevice& device)
      : Device(device)
      , Header()
    {}

    ~WaveBuffer() override
    {
      try
      {
        Reset();
      }
      catch (const Error& e)
      {
        Dbg("Failed to reset buffer: {}", e.ToString());
      }
    }

    void Write(const Chunk& buf) override
    {
      PrepareBuffer(buf.size());
      /*

       From http://msdn.microsoft.com/en-us/library/windows/desktop/dd797880(v=vs.85).aspx :

       For 8-bit PCM data, each sample is represented by a single unsigned data byte.
       For 16-bit PCM data, each sample is represented by a 16-bit signed value.
      */
      if (Sample::BITS == 16)
      {
        buf.ToS16(Header.lpData);
      }
      else
      {
        buf.ToU8(Header.lpData);
      }
      Header.dwFlags &= ~WHDR_DONE;
      Device.Write(Header);
    }

  private:
    void PrepareBuffer(std::size_t samples)
    {
      if (samples <= Buffer.size())
      {
        WaitForBufferDone();
      }
      else
      {
        Allocate(samples);
      }
      assert(Header.dwFlags & WHDR_DONE);
      Header.dwBufferLength = static_cast< ::DWORD>(samples * sizeof(Buffer.front()));
    }

    void Allocate(std::size_t samples)
    {
      Reset();
      Buffer.resize(samples);
      Header.lpData = ::LPSTR(Buffer.data());
      Header.dwBufferLength = ::DWORD(Buffer.size()) * sizeof(Buffer.front());
      Header.dwUser = Header.dwLoops = Header.dwFlags = 0;
      Device.PrepareHeader(Header);
      Require(IsPrepared());
      // mark as free
      Header.dwFlags |= WHDR_DONE;
    }

    void Reset()
    {
      if (IsPrepared())
      {
        WaitForBufferDone();
        // safe to call more than once
        Device.UnprepareHeader(Header);
        Require(!IsPrepared());
      }
    }

    void WaitForBufferDone()
    {
      while (!(Header.dwFlags & WHDR_DONE))
      {
        Device.WaitForBufferComplete();
      }
    }

    bool IsPrepared() const
    {
      return 0 != (Header.dwFlags & WHDR_PREPARED);
    }

  private:
    WaveOutDevice& Device;
    Chunk Buffer;
    ::WAVEHDR Header;
  };

  class CycledWaveBuffer : public WaveTarget
  {
  public:
    CycledWaveBuffer(WaveOutDevice::Ptr device, std::size_t count)
      : Device(std::move(device))
      , Buffers(count)
    {
      for (auto& buf : Buffers)
      {
        buf = MakePtr<WaveBuffer>(*Device);
      }
    }

    ~CycledWaveBuffer() override
    {
      try
      {
        Reset();
      }
      catch (const Error& e)
      {
        Dbg("Failed to reset cycle buffer: {}", e.ToString());
      }
    }

    void Write(const Chunk& buf) override
    {
      Buffers[Cursor]->Write(buf);
      Cursor = (Cursor + 1) % Buffers.size();
    }

  private:
    void Reset()
    {
      BuffersArray().swap(Buffers);
      Cursor = 0;
    }

  private:
    const WaveOutDevice::Ptr Device;
    using BuffersArray = std::vector<WaveTarget::Ptr>;
    BuffersArray Buffers;
    std::size_t Cursor = 0;
  };

  // volume controller implementation
  class VolumeControl : public Sound::VolumeControl
  {
  public:
    explicit VolumeControl(WaveOutDevice::Ptr device)
      : Device(std::move(device))
    {
      Dbg("Created volume controller");
    }

    Gain GetVolume() const override
    {
      DWORD buffer = 0;
      Device->GetVolume(&buffer);
      return {DecodeVolume(uint16_t(buffer & 0xffff)), DecodeVolume(uint16_t(buffer >> 16))};
    }

    void SetVolume(const Gain& volume) override
    {
      if (!volume.IsNormalized())
      {
        throw Error(THIS_LINE, translate("Failed to set volume: gain is out of range."));
      }
      const DWORD buffer = EncodeVolume(volume.Left()) | (DWORD(EncodeVolume(volume.Right())) << 16);
      Device->SetVolume(buffer);
    }

  private:
    static Gain::Type DecodeVolume(uint16_t t)
    {
      return {int_t(t), MAX_WIN32_VOLUME};
    }

    static uint16_t EncodeVolume(Gain::Type t)
    {
      return static_cast<uint16_t>((t * MAX_WIN32_VOLUME).Round());
    }

  private:
    const WaveOutDevice::Ptr Device;
  };

  class BackendParameters
  {
  public:
    explicit BackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {}

    int_t GetDevice() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Win32;
      return Parameters::GetInteger<int_t>(Accessor, DEVICE, DEVICE_DEFAULT);
    }

    uint_t GetBuffers() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Win32;
      const auto buffers = Parameters::GetInteger<uint_t>(Accessor, BUFFERS, BUFFERS_DEFAULT);
      if (!Math::InRange(buffers, BUFFERS_MIN, BUFFERS_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
                                 translate("Win32 backend error: buffers count ({0}) is out of range ({1}..{2})."),
                                 buffers, BUFFERS_MIN, BUFFERS_MAX);
      }
      return buffers;
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Api::Ptr api, Parameters::Accessor::Ptr params)
      : WinApi(std::move(api))
      , BackendParams(std::move(params))
      , RenderingParameters(RenderParameters::Create(BackendParams))
    {}

    ~BackendWorker() override
    {
      assert(!Objects.Device || !"Win32Backend::Stop should be called before exit");
    }

    void Startup() override
    {
      Dbg("Starting");
      Objects = OpenDevices();
      Dbg("Started");
    }

    void Shutdown() override
    {
      Dbg("Stopping");
      Objects.Volume.reset();
      Objects.Target.reset();
      Objects.Device.reset();
      Dbg("Stopped");
    }

    void Pause() override
    {
      Objects.Device->Pause();
    }

    void Resume() override
    {
      Objects.Device->Resume();
    }

    void FrameStart(const Module::State& /*state*/) override {}

    void FrameFinish(Chunk buffer) override
    {
      Objects.Target->Write(buffer);
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return CreateVolumeControlDelegate(Objects.Volume);
    }

  private:
    ::WAVEFORMATEX GetFormat() const
    {
      ::WAVEFORMATEX format;
      std::memset(&format, 0, sizeof(format));
      format.wFormatTag = WAVE_FORMAT_PCM;
      format.nChannels = Sample::CHANNELS;
      format.nSamplesPerSec = static_cast< ::DWORD>(RenderingParameters->SoundFreq());
      format.nBlockAlign = sizeof(Sample);
      format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
      format.wBitsPerSample = Sample::BITS;
      return format;
    }

    struct WaveOutObjects
    {
      WaveOutDevice::Ptr Device;
      WaveTarget::Ptr Target;
      VolumeControl::Ptr Volume;
    };

    WaveOutObjects OpenDevices() const
    {
      WaveOutObjects res;
      const BackendParameters params(*BackendParams);
      res.Device = MakePtr<WaveOutDevice>(WinApi, GetFormat(), params.GetDevice());
      res.Target = MakePtr<CycledWaveBuffer>(res.Device, params.GetBuffers());
      res.Volume = MakePtr<VolumeControl>(res.Device);
      return res;
    }

  private:
    const Api::Ptr WinApi;
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    WaveOutObjects Objects;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : WinApi(std::move(api))
    {}

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr /*holder*/) const override
    {
      return MakePtr<BackendWorker>(WinApi, params);
    }

  private:
    const Api::Ptr WinApi;
  };

  class WaveDevice : public Device
  {
  public:
    WaveDevice(Api::Ptr api, int_t id)
      : WinApi(std::move(api))
      , IdValue(id)
    {}

    int_t Id() const override
    {
      return IdValue;
    }

    String Name() const override
    {
      WAVEOUTCAPSW caps;
      if (MMSYSERR_NOERROR != WinApi->waveOutGetDevCapsW(static_cast<UINT>(IdValue), &caps, sizeof(caps)))
      {
        Dbg("Failed to get device name");
        return {};
      }
      const auto* const name = caps.szPname;
      static_assert(sizeof(*name) == sizeof(uint16_t), "Wide char size mismatch");
      return Strings::Utf16ToUtf8(safe_ptr_cast<const uint16_t*>(name));
    }

  private:
    const Api::Ptr WinApi;
    const int_t IdValue;
  };

  class DevicesIterator : public Device::Iterator
  {
  public:
    explicit DevicesIterator(Api::Ptr api)
      : WinApi(std::move(api))
      , Limit(WinApi->waveOutGetNumDevs())
    {
      if (Limit)
      {
        Dbg("Detected {} devices to output.", Limit);
        Current = -1;  // WAVE_MAPPER
      }
      else
      {
        Current = 0;
      }
    }

    bool IsValid() const override
    {
      return Current != Limit;
    }

    Device::Ptr Get() const override
    {
      return IsValid() ? MakePtr<WaveDevice>(WinApi, Current) : Device::Ptr();
    }

    void Next() override
    {
      if (IsValid())
      {
        ++Current;
      }
    }

  private:
    const Api::Ptr WinApi;
    const int_t Limit;
    int_t Current = 0;
  };
}  // namespace Sound::Win32

namespace Sound
{
  void RegisterWin32Backend(BackendsStorage& storage)
  {
    try
    {
      auto api = Win32::LoadDynamicApi();
      if (Win32::DevicesIterator(api).IsValid())
      {
        auto factory = MakePtr<Win32::BackendWorkerFactory>(std::move(api));
        storage.Register(Win32::BACKEND_ID, Win32::BACKEND_DESCRIPTION, Win32::CAPABILITIES, std::move(factory));
      }
      else
      {
        throw Error(THIS_LINE, translate("No suitable output devices found"));
      }
    }
    catch (const Error& e)
    {
      storage.Register(Win32::BACKEND_ID, Win32::BACKEND_DESCRIPTION, Win32::CAPABILITIES, e);
    }
  }

  namespace Win32
  {
    Device::Iterator::Ptr EnumerateDevices()
    {
      try
      {
        auto api = LoadDynamicApi();
        return MakePtr<DevicesIterator>(std::move(api));
      }
      catch (const Error& e)
      {
        Dbg("{}", e.ToString());
        return Device::Iterator::CreateStub();
      }
    }
  }  // namespace Win32
}  // namespace Sound
