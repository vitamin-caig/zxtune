/*
Abstract:
  Win32 waveout backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "win32.h"
#include "win32_api.h"
#include "backend_impl.h"
#include "enumerator.h"
#include "volume_control.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//std includes
#include <algorithm>
#include <cstring>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
//text includes
#include "text/backends.h"

#define FILE_TAG 5E3F141A

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Debug::Stream Dbg("Sound::Backend::Win32");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;

  const uint_t MAX_WIN32_VOLUME = 0xffff;
  const uint_t BUFFERS_MIN = 3;
  const uint_t BUFFERS_MAX = 10;

  /*

   From http://msdn.microsoft.com/en-us/library/windows/desktop/dd797880(v=vs.85).aspx :

   For 8-bit PCM data, each sample is represented by a single unsigned data byte.
   For 16-bit PCM data, each sample is represented by a 16-bit signed value.
  */

  const bool SamplesShouldBeConverted = (sizeof(Sample) > 1) != SAMPLE_SIGNED;

  class SharedEvent
  {
  public:
    SharedEvent()
      : Handle(::CreateEvent(0, FALSE, FALSE, 0), &::CloseHandle)
    {
    }

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
        //TODO: convert code to string
        throw MakeFormattedError(loc, translate("Error in Win32 backend: code %1%."), ::GetLastError());
      }
    }
  private:
    const boost::shared_ptr<void> Handle;
  };

  //lightweight wrapper around HWAVEOUT handle
  class WaveOutDevice
  {
  public:
    typedef boost::shared_ptr<WaveOutDevice> Ptr;
    typedef boost::weak_ptr<WaveOutDevice> WeakPtr;

    WaveOutDevice(Win32::Api::Ptr api, const ::WAVEFORMATEX& format, UINT device)
      : Api(api)
      , Handle(0)
    {
      Dbg("Opening device %1% (%2% Hz)", device, format.nSamplesPerSec);
      CheckMMResult(Api->waveOutOpen(&Handle, device, &format, DWORD_PTR(Event.Get()), 0,
        CALLBACK_EVENT | WAVE_FORMAT_DIRECT), THIS_LINE);
    }

    ~WaveOutDevice()
    {
      try
      {
        Close();
      }
      catch (const Error& e)
      {
        Dbg("Failed to close device: %1%", e.ToString());
      }
    }

    void Close()
    {
      if (Handle)
      {
        Dbg("Closing device");
        CheckMMResult(Api->waveOutReset(Handle), THIS_LINE);
        CheckMMResult(Api->waveOutClose(Handle), THIS_LINE);
        Handle = 0;
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
      CheckMMResult(Api->waveOutPrepareHeader(Handle, &header, sizeof(header)), THIS_LINE);
    }

    void UnprepareHeader(::WAVEHDR& header)
    {
      CheckMMResult(Api->waveOutUnprepareHeader(Handle, &header, sizeof(header)), THIS_LINE);
    }

    void Write(::WAVEHDR& header)
    {
      CheckMMResult(Api->waveOutWrite(Handle, &header, sizeof(header)), THIS_LINE);
    }

    void Pause()
    {
      CheckMMResult(Api->waveOutPause(Handle), THIS_LINE);
    }

    void Resume()
    {
      CheckMMResult(Api->waveOutRestart(Handle), THIS_LINE);
    }

    void GetVolume(LPDWORD val)
    {
      CheckMMResult(Api->waveOutGetVolume(Handle, val), THIS_LINE);
    }

    void SetVolume(DWORD val)
    {
      CheckMMResult(Api->waveOutSetVolume(Handle, val), THIS_LINE);
    }
  private:
    void CheckMMResult(::MMRESULT res, Error::LocationRef loc) const
    {
      if (MMSYSERR_NOERROR != res)
      {
        std::vector<char> buffer(1024);
        if (MMSYSERR_NOERROR == Api->waveOutGetErrorTextA(res, &buffer[0], static_cast<UINT>(buffer.size())))
        {
          throw MakeFormattedError(loc, translate("Error in Win32 backend: %1%."),
            String(buffer.begin(), std::find(buffer.begin(), buffer.end(), '\0')));
        }
        else
        {
          throw MakeFormattedError(loc, translate("Error in Win32 backend: code %1%."), res);
        }
      }
    }
  private:
    const Win32::Api::Ptr Api;
    const SharedEvent Event;
    ::HWAVEOUT Handle;
  };

  class WaveTarget
  {
  public:
    typedef boost::shared_ptr<WaveTarget> Ptr;
    virtual ~WaveTarget() {}

    virtual std::size_t Write(const OutputSample* buf, std::size_t samples) = 0;
  };

  class WaveBuffer : public WaveTarget
  {
  public:
    explicit WaveBuffer(WaveOutDevice::Ptr device)
      : Device(device)
      , Header()
    {
    }

    virtual ~WaveBuffer()
    {
      try
      {
        Reset();
      }
      catch (const Error& e)
      {
        Dbg("Failed to reset buffer: %1%", e.ToString());
      }
    }

    virtual std::size_t Write(const OutputSample* buf, std::size_t samples)
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
      const std::size_t toWrite = std::min<std::size_t>(samples, Buffer.size());
      Header.dwBufferLength = static_cast< ::DWORD>(toWrite * sizeof(Buffer.front()));
      if (SamplesShouldBeConverted)
      {
        ChangeSignCopy(buf, buf + toWrite, safe_ptr_cast<OutputSample*>(Header.lpData));
      }
      else
      {
        std::memcpy(Header.lpData, buf, toWrite * sizeof(*buf));
      }
      Header.dwFlags &= ~WHDR_DONE;
      Device->Write(Header);
      return toWrite;
    }
  private:
    void Allocate(std::size_t size)
    {
      Reset();
      Buffer.resize(size);
      Header.lpData = ::LPSTR(&Buffer[0]);
      Header.dwBufferLength = ::DWORD(Buffer.size()) * sizeof(Buffer.front());
      Header.dwUser = Header.dwLoops = Header.dwFlags = 0;
      Device->PrepareHeader(Header);
      Require(IsPrepared());
      //mark as free
      Header.dwFlags |= WHDR_DONE;
    }

    void Reset()
    {
      if (IsPrepared())
      {
        WaitForBufferDone();
        // safe to call more than once
        Device->UnprepareHeader(Header);
        Require(!IsPrepared());
      }
    }

    void WaitForBufferDone()
    {
      while (!(Header.dwFlags & WHDR_DONE))
      {
        Device->WaitForBufferComplete();
      }
    }

    bool IsPrepared() const
    {
      return 0 != (Header.dwFlags & WHDR_PREPARED);
    }
  private:
    const WaveOutDevice::Ptr Device;
    Chunk Buffer;
    ::WAVEHDR Header;
  };

  class CycledWaveBuffer : public WaveTarget
  {
  public:
    CycledWaveBuffer(WaveOutDevice::Ptr device, std::size_t count)
      : Buffers(count)
      , Cursor()
    {
      for (BuffersArray::iterator it = Buffers.begin(), lim = Buffers.end(); it != lim; ++it)
      {
        *it = boost::make_shared<WaveBuffer>(device);
      }
    }

    virtual ~CycledWaveBuffer()
    {
      try
      {
        Reset();
      }
      catch (const Error& e)
      {
        Dbg("Failed to reset cycle buffer: %1%", e.ToString());
      }
    }

    virtual std::size_t Write(const OutputSample* buf, std::size_t samples)
    {
      // split big buffer
      // small buffer is covered by adjusting of subbuffers
      std::size_t done = 0;
      while (done < samples)
      {
        const std::size_t written = Buffers[Cursor]->Write(buf, samples);
        Cursor = (Cursor + 1) % Buffers.size();
        done += written;
        buf += written;
      }
      return done;
    }
  private:
    void Reset()
    {
      BuffersArray().swap(Buffers);
      Cursor = 0;
    }
  private:
    typedef std::vector<WaveTarget::Ptr> BuffersArray;
    BuffersArray Buffers;
    std::size_t Cursor;
  };

  // volume controller implementation
  class Win32VolumeController : public VolumeControl
  {
  public:
    explicit Win32VolumeController(WaveOutDevice::Ptr device)
      : Device(device)
    {
      Dbg("Created volume controller");
    }

    virtual MultiGain GetVolume() const
    {
      boost::array<uint16_t, OUTPUT_CHANNELS> buffer;
      BOOST_STATIC_ASSERT(sizeof(buffer) == sizeof(DWORD));
      Device->GetVolume(safe_ptr_cast<LPDWORD>(&buffer[0]));
      MultiGain volume;
      std::transform(buffer.begin(), buffer.end(), volume.begin(), std::bind2nd(std::divides<Gain>(), MAX_WIN32_VOLUME));
      return volume;
    }

    virtual void SetVolume(const MultiGain& volume)
    {
      if (volume.end() != std::find_if(volume.begin(), volume.end(), std::bind2nd(std::greater<Gain>(), Gain(1.0))))
      {
        throw Error(THIS_LINE, translate("Failed to set volume: gain is out of range."));
      }
      boost::array<uint16_t, OUTPUT_CHANNELS> buffer;
      std::transform(volume.begin(), volume.end(), buffer.begin(), std::bind2nd(std::multiplies<Gain>(), Gain(MAX_WIN32_VOLUME)));
      BOOST_STATIC_ASSERT(sizeof(buffer) == sizeof(DWORD));
      Device->SetVolume(*safe_ptr_cast<LPDWORD>(&buffer[0]));
    }
  private:
    const WaveOutDevice::Ptr Device;
  };

  class Win32BackendParameters
  {
  public:
    explicit Win32BackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    int_t GetDevice() const
    {
      Parameters::IntType device = Parameters::ZXTune::Sound::Backends::Win32::DEVICE_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::Sound::Backends::Win32::DEVICE, device);
      return static_cast<int_t>(device);
    }

    std::size_t GetBuffers() const
    {
      Parameters::IntType buffers = Parameters::ZXTune::Sound::Backends::Win32::BUFFERS_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Sound::Backends::Win32::BUFFERS, buffers) &&
          !Math::InRange<Parameters::IntType>(buffers, BUFFERS_MIN, BUFFERS_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("Win32 backend error: buffers count (%1%) is out of range (%2%..%3%)."), static_cast<int_t>(buffers), BUFFERS_MIN, BUFFERS_MAX);
      }
      return static_cast<std::size_t>(buffers);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class Win32BackendWorker : public BackendWorker
                           , private boost::noncopyable
  {
  public:
    Win32BackendWorker(Win32::Api::Ptr api, Parameters::Accessor::Ptr params)
      : Api(api)
      , BackendParams(params)
      , RenderingParameters(RenderParameters::Create(BackendParams))
    {
    }

    virtual ~Win32BackendWorker()
    {
      assert(!Objects.Device || !"Win32Backend::Stop should be called before exit");
    }

    virtual void Test()
    {
      const WaveOutObjects obj = OpenDevices();
      obj.Device->Close();
    }

    virtual void Startup()
    {
      Dbg("Starting");
      Objects = OpenDevices();
      Dbg("Started");
    }

    virtual void Shutdown()
    {
      Dbg("Stopping");
      Objects.Volume.reset();
      Objects.Target.reset();
      Objects.Device.reset();
      Dbg("Stopped");
    }

    virtual void Pause()
    {
      Objects.Device->Pause();
    }

    virtual void Resume()
    {
      Objects.Device->Resume();
    }

    virtual void BufferReady(Chunk& buffer)
    {
      Objects.Target->Write(&buffer.front(), buffer.size());
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return CreateVolumeControlDelegate(Objects.Volume);
    }
  private:
    ::WAVEFORMATEX GetFormat() const
    {
      ::WAVEFORMATEX format;
      std::memset(&format, 0, sizeof(format));
      format.wFormatTag = WAVE_FORMAT_PCM;
      format.nChannels = OUTPUT_CHANNELS;
      format.nSamplesPerSec = static_cast< ::DWORD>(RenderingParameters->SoundFreq());
      format.nBlockAlign = sizeof(OutputSample);
      format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
      format.wBitsPerSample = 8 * sizeof(Sample);
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
      const Win32BackendParameters params(*BackendParams);
      res.Device = boost::make_shared<WaveOutDevice>(Api, GetFormat(), params.GetDevice());
      res.Target = boost::make_shared<CycledWaveBuffer>(res.Device, params.GetBuffers());
      res.Volume = boost::make_shared<Win32VolumeController>(res.Device);
      return res;
    }
  private:
    const Win32::Api::Ptr Api;
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    WaveOutObjects Objects;
  };

  const String ID = Text::WIN32_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("Win32 sound system backend");

  class Win32BackendCreator : public BackendCreator
  {
  public:
    explicit Win32BackendCreator(Win32::Api::Ptr api)
      : Api(api)
    {
    }

    virtual String Id() const
    {
      return ID;
    }

    virtual String Description() const
    {
      return translate(DESCRIPTION);
    }

    virtual uint_t Capabilities() const
    {
      return CAPABILITIES;
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const BackendWorker::Ptr worker(new Win32BackendWorker(Api, allParams));
        return Sound::CreateBackend(params, worker);
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE,
          translate("Failed to create backend '%1%'."), Id()).AddSuberror(e);
      }
    }
  private:
    const Win32::Api::Ptr Api;
  };

  class Win32Device : public Win32::Device
  {
  public:
    Win32Device(Win32::Api::Ptr api, int_t id)
      : Api(api)
      , IdValue(id)
    {
    }

    virtual int_t Id() const
    {
      return IdValue;
    }

    virtual String Name() const
    {
      WAVEOUTCAPSA caps;
      if (MMSYSERR_NOERROR != Api->waveOutGetDevCapsA(static_cast<UINT>(IdValue), &caps, sizeof(caps)))
      {
        Dbg("Failed to get device name");
        return String();
      }
      return FromStdString(caps.szPname);
    }
  private:
    const Win32::Api::Ptr Api;
    const int_t IdValue;
  };

  class DevicesIterator : public Win32::Device::Iterator
  {
  public:
    explicit DevicesIterator(Win32::Api::Ptr api)
      : Api(api)
      , Limit(Api->waveOutGetNumDevs())
      , Current()
    {
      if (Limit)
      {
        Dbg("Detected %1% devices to output.", Limit);
        Current = -1;//WAVE_MAPPER
      }
      else
      {
        Current = 0;
      }
    }

    virtual bool IsValid() const
    {
      return Current != Limit;
    }

    virtual Win32::Device::Ptr Get() const
    {
      return IsValid()
        ? boost::make_shared<Win32Device>(Api, Current)
        : Win32::Device::Ptr();
    }

    virtual void Next()
    {
      if (IsValid())
      {
        ++Current;
      }
    }
  private:
    const Win32::Api::Ptr Api;
    const int_t Limit;
    int_t Current;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterWin32Backend(BackendsEnumerator& enumerator)
    {
      try
      {
        const Win32::Api::Ptr api = Win32::LoadDynamicApi();
        if (DevicesIterator(api).IsValid())
        {
          const BackendCreator::Ptr creator(new Win32BackendCreator(api));
          enumerator.RegisterCreator(creator);
        }
        else
        {
          throw Error(THIS_LINE, translate("No suitable output devices found"));
        }
      }
      catch (const Error& e)
      {
        enumerator.RegisterCreator(CreateUnavailableBackendStub(ID, DESCRIPTION, CAPABILITIES, e));
      }
    }

    namespace Win32
    {
      Device::Iterator::Ptr EnumerateDevices()
      {
        try
        {
          const Win32::Api::Ptr api = Win32::LoadDynamicApi();
          return Device::Iterator::Ptr(new DevicesIterator(api));
        }
        catch (const Error& e)
        {
          Dbg("%1%", e.ToString());
          return Device::Iterator::CreateStub();
        }
      }
    }
  }
}
