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
//common includes
#include <contract.h>
#include <error_tools.h>
#include <logging.h>
#include <shared_library_gate.h>
#include <tools.h>
//library includes
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//platform-dependent includes
#include <windows.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ref.hpp>
#include <boost/thread/thread.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 5E3F141A

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::Win32");

  const Char WIN32_BACKEND_ID[] = {'w', 'i', 'n', '3', '2', 0};

  const uint_t MAX_WIN32_VOLUME = 0xffff;
  const uint_t BUFFERS_MIN = 3;
  const uint_t BUFFERS_MAX = 10;

  /*

   From http://msdn.microsoft.com/en-us/library/windows/desktop/dd797880(v=vs.85).aspx :

   For 8-bit PCM data, each sample is represented by a single unsigned data byte.
   For 16-bit PCM data, each sample is represented by a 16-bit signed value.
  */

  const bool SamplesShouldBeConverted = (sizeof(Sample) > 1) != SAMPLE_SIGNED;

  inline Sample ConvertSample(Sample in)
  {
    return SamplesShouldBeConverted
      ? ToSignedSample(in)
      : in;
  }

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
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_WIN32_BACKEND_ERROR, ::GetLastError());
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
      Log::Debug(THIS_MODULE, "Opening device %1% (%2% Hz)", device, format.nSamplesPerSec);
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
        Log::Debug(THIS_MODULE, "Failed to close device: %1%", Error::ToString(e));
      }
    }

    void Close()
    {
      if (Handle)
      {
        Log::Debug(THIS_MODULE, "Closing device");
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
          throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_WIN32_BACKEND_ERROR,
            String(buffer.begin(), std::find(buffer.begin(), buffer.end(), '\0')));
        }
        else
        {
          throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_WIN32_BACKEND_ERROR, res);
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

    virtual std::size_t Write(const MultiSample* buf, std::size_t samples) = 0;
  };

  class WaveBuffer : public WaveTarget
  {
  public:
    WaveBuffer(WaveOutDevice::Ptr device, std::size_t size)
      : Device(device)
      , Buffer(size)
      , Header()
    {
      Allocate();
    }

    virtual ~WaveBuffer()
    {
      try
      {
        Reset();
      }
      catch (const Error& e)
      {
        Log::Debug(THIS_MODULE, "Failed to reset buffer: %1%", Error::ToString(e));
      }
    }

    virtual std::size_t Write(const MultiSample* buf, std::size_t samples)
    {
      WaitForBufferDone();
      assert(Header.dwFlags & WHDR_DONE);
      const std::size_t toWrite = std::min<std::size_t>(samples, Buffer.size());
      Header.dwBufferLength = static_cast< ::DWORD>(toWrite * sizeof(Buffer.front()));
      std::transform(&buf->front(), &(buf + toWrite)->front(), safe_ptr_cast<Sample*>(Header.lpData), &ConvertSample);
      Header.dwFlags &= ~WHDR_DONE;
      Device->Write(Header);
      return toWrite;
    }
  private:
    void Allocate()
    {
      Header.lpData = ::LPSTR(&Buffer[0]);
      Header.dwBufferLength = ::DWORD(Buffer.size()) * sizeof(Buffer.front());
      Header.dwUser = Header.dwLoops = Header.dwFlags = 0;
      Device->PrepareHeader(Header);
      //mark as free
      Header.dwFlags |= WHDR_DONE;
    }

    void Reset()
    {
      WaitForBufferDone();
      // safe to call more than once
      Device->UnprepareHeader(Header);
      Require(0 == (Header.dwFlags & WHDR_PREPARED));
    }

    void WaitForBufferDone()
    {
      while (!(Header.dwFlags & WHDR_DONE))
      {
        Device->WaitForBufferComplete();
      }
    }
  private:
    const WaveOutDevice::Ptr Device;
    const Chunk Buffer;
    ::WAVEHDR Header;
  };

  class CycledWaveBuffer : public WaveTarget
  {
  public:
    CycledWaveBuffer(WaveOutDevice::Ptr device, std::size_t size, std::size_t count)
      : Buffers(count)
      , Cursor()
    {
      for (BuffersArray::iterator it = Buffers.begin(), lim = Buffers.end(); it != lim; ++it)
      {
        *it = boost::make_shared<WaveBuffer>(device, size);
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
        Log::Debug(THIS_MODULE, "Failed to reset cycle buffer: %1%", Error::ToString(e));
      }
    }

    virtual std::size_t Write(const MultiSample* buf, std::size_t samples)
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
      Log::Debug(THIS_MODULE, "Created volume controller");
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      // use exceptions for simplification
      try
      {
        if (WaveOutDevice::Ptr dev = Device.lock())
        {
          boost::array<uint16_t, OUTPUT_CHANNELS> buffer;
          BOOST_STATIC_ASSERT(sizeof(buffer) == sizeof(DWORD));
          dev->GetVolume(safe_ptr_cast<LPDWORD>(&buffer[0]));
          std::transform(buffer.begin(), buffer.end(), volume.begin(), std::bind2nd(std::divides<Gain>(), MAX_WIN32_VOLUME));
        }
        return Error();//?
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
      // use exceptions for simplification
      try
      {
        if (WaveOutDevice::Ptr dev = Device.lock())
        {
          boost::array<uint16_t, OUTPUT_CHANNELS> buffer;
          std::transform(volume.begin(), volume.end(), buffer.begin(), std::bind2nd(std::multiplies<Gain>(), Gain(MAX_WIN32_VOLUME)));
          BOOST_STATIC_ASSERT(sizeof(buffer) == sizeof(DWORD));
          dev->SetVolume(*safe_ptr_cast<LPDWORD>(&buffer[0]));
        }
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }
  private:
    const WaveOutDevice::WeakPtr Device;
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
          !in_range<Parameters::IntType>(buffers, BUFFERS_MIN, BUFFERS_MAX))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_WIN32_BACKEND_INVALID_BUFFERS, static_cast<int_t>(buffers), BUFFERS_MIN, BUFFERS_MAX);
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
      , Volume(boost::make_shared<Win32VolumeController>(Device))
    {
    }

    virtual ~Win32BackendWorker()
    {
      assert(!Device || !"Win32Backend::Stop should be called before exit");
    }

    virtual void Test()
    {
      const Win32BackendParameters params(*BackendParams);
      WaveOutDevice device(Api, GetFormat(), params.GetDevice());
      device.Close();
    }

    virtual void OnStartup(const Module::Holder& /*module*/)
    {
      assert(!Device);

      const Win32BackendParameters params(*BackendParams);
      const WaveOutDevice::Ptr newDevice = boost::make_shared<WaveOutDevice>(Api, GetFormat(), params.GetDevice());
      const WaveTarget::Ptr newTarget = boost::make_shared<CycledWaveBuffer>(newDevice, RenderingParameters->SamplesPerFrame(), params.GetBuffers());
      const VolumeControl::Ptr newVolume = boost::make_shared<Win32VolumeController>(newDevice);
      const boost::mutex::scoped_lock lock(StateMutex);
      Target = newTarget;
      Device = newDevice;
      Volume = newVolume;
    }

    virtual void OnShutdown()
    {
      Target = WaveTarget::Ptr();
      Device = WaveOutDevice::Ptr();
      //keep volume controller available
    }

    virtual void OnPause()
    {
      Device->Pause();
    }

    virtual void OnResume()
    {
      Device->Resume();
    }

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }

    virtual void OnBufferReady(Chunk& buffer)
    {
      Target->Write(&buffer.front(), buffer.size());
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return Volume;
    }
  private:
    ::WAVEFORMATEX GetFormat() const
    {
      ::WAVEFORMATEX format;
      std::memset(&format, 0, sizeof(format));
      format.wFormatTag = WAVE_FORMAT_PCM;
      format.nChannels = OUTPUT_CHANNELS;
      format.nSamplesPerSec = static_cast< ::DWORD>(RenderingParameters->SoundFreq());
      format.nBlockAlign = sizeof(MultiSample);
      format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
      format.wBitsPerSample = 8 * sizeof(Sample);
      return format;
    }
  private:
    const Win32::Api::Ptr Api;
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    boost::mutex StateMutex;
    WaveOutDevice::Ptr Device;
    WaveTarget::Ptr Target;
    VolumeControl::Ptr Volume;
  };

  class Win32BackendCreator : public BackendCreator
  {
  public:
    explicit Win32BackendCreator(Win32::Api::Ptr api)
      : Api(api)
    {
    }

    virtual String Id() const
    {
      return WIN32_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::WIN32_BACKEND_DESCRIPTION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const BackendWorker::Ptr worker(new Win32BackendWorker(Api, allParams));
        result = Sound::CreateBackend(params, worker);
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_FAILED_CREATE,
          Text::SOUND_ERROR_BACKEND_FAILED, Id()).AddSuberror(e);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
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
      WAVEOUTCAPS caps;
      if (MMSYSERR_NOERROR != Api->waveOutGetDevCapsA(static_cast<UINT>(IdValue), &caps, sizeof(caps)))
      {
        Log::Debug(THIS_MODULE, "Failed to get device name");
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
        Log::Debug(THIS_MODULE, "Detected %1% devices to output.", Limit);
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
          Log::Debug(THIS_MODULE, "No devices detected. Disable backend.");
        }
      }
      catch (const Error& e)
      {
        Log::Debug(THIS_MODULE, "%1%", Error::ToString(e));
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
          Log::Debug(THIS_MODULE, "%1%", Error::ToString(e));
          return Device::Iterator::CreateStub();
        }
      }
    }
  }
}
