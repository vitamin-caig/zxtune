/*
Abstract:
  DirectSound backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef DIRECTSOUND_SUPPORT

//local includes
#include "backend_impl.h"
#include "enumerator.h"
//common includes
#include <error_tools.h>
#include <logging.h>
#include <shared_library_gate.h>
#include <tools.h>
//library includes
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
//platform-dependent includes
#define NOMINMAX
#include <dsound.h>
//std includes
#include <cmath>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG BCBCECCC

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::DirectSound");

  const Char DSOUND_BACKEND_ID[] = {'d', 's', 'o', 'u', 'n', 'd', 0};

  const uint_t LATENCY_MIN = 20;
  const uint_t LATENCY_MAX = 10000;

  struct DirectSoundLibraryTraits
  {
    static std::string GetName()
    {
      return "dsound";
    }

    static void Startup()
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    static void Shutdown()
    {
      Log::Debug(THIS_MODULE, "Library unloaded");
    }
  };

  typedef SharedLibraryGate<DirectSoundLibraryTraits> DirectSoundLibrary;

  HWND GetWindowHandle()
  {
    if (HWND res = ::GetForegroundWindow())
    {
      return res;
    }
    return ::GetDesktopWindow();
  }

  void CheckWin32Error(HRESULT res, Error::LocationRef loc)
  {
    if (FAILED(res))
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_DSOUND_BACKEND_ERROR, res);
    }
  }

  void ReleaseRef(IUnknown* iface)
  {
    if (iface)
    {
      iface->Release();
    }
  }

  typedef boost::shared_ptr<IDirectSound> DirectSoundDevicePtr;
  typedef boost::shared_ptr<IDirectSoundBuffer> DirectSoundBufferPtr;

  DirectSoundDevicePtr OpenDevice(/*device*/)
  {
    Log::Debug(THIS_MODULE, "OpenDevice");
    DirectSoundDevicePtr::pointer raw = 0;
    CheckWin32Error(::DirectSoundCreate(NULL/*device*/, &raw, NULL), THIS_LINE);
    const DirectSoundDevicePtr result = DirectSoundDevicePtr(raw, &ReleaseRef);
    CheckWin32Error(result->SetCooperativeLevel(GetWindowHandle(), DSSCL_PRIORITY), THIS_LINE);
    Log::Debug(THIS_MODULE, "Opened");
    return result;
  }

  DirectSoundBufferPtr CreateBuffer(DirectSoundDevicePtr device, uint_t sampleRate, uint_t bufferInMs)
  {
    Log::Debug(THIS_MODULE, "CreateBuffer");
    WAVEFORMATEX format;
    std::memset(&format, 0, sizeof(format));
    format.cbSize = sizeof(format);
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = OUTPUT_CHANNELS;
    format.nSamplesPerSec = static_cast< ::DWORD>(sampleRate);
    format.nBlockAlign = sizeof(MultiSample);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.wBitsPerSample = 8 * sizeof(Sample);

    DSBUFFERDESC buffer;
    std::memset(&buffer, 0, sizeof(buffer));
    buffer.dwSize = sizeof(buffer);
    buffer.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
    buffer.dwBufferBytes = format.nAvgBytesPerSec * bufferInMs / 1000;
    buffer.lpwfxFormat = &format;

    LPDIRECTSOUNDBUFFER rawPrimary = 0;
    CheckWin32Error(device->CreateSoundBuffer(&buffer, &rawPrimary, NULL), THIS_LINE);
    assert(rawPrimary);
    const boost::shared_ptr<IDirectSoundBuffer> primary(rawPrimary, &ReleaseRef);
    Log::Debug(THIS_MODULE, "Created");
    return primary;
  }

  class StreamBuffer
  {
  public:
    typedef boost::shared_ptr<StreamBuffer> Ptr;

    StreamBuffer(DirectSoundBufferPtr buff, boost::posix_time::millisec sleepPeriod)
      : Buff(buff)
      , SleepPeriod(sleepPeriod)
      , BuffSize(0)
      , Cursor(0)
    {
      DSBCAPS caps;
      std::memset(&caps, 0, sizeof(caps));
      caps.dwSize = sizeof(caps);
      CheckWin32Error(Buff->GetCaps(&caps), THIS_LINE);
      BuffSize = caps.dwBufferBytes;
      Log::Debug(THIS_MODULE, "Using cycle buffer size %1%", BuffSize);
      CheckWin32Error(Buff->Play(0, 0, DSBPLAY_LOOPING), THIS_LINE);
    }

    void Add(const std::vector<MultiSample>& buffer)
    {
      const std::size_t srcSize = buffer.size() * sizeof(buffer.front());

      if (!WaitForFree(srcSize))
      {
        return;
      }

      LPVOID part1Data = 0, part2Data = 0;
      DWORD part1Size = 0, part2Size = 0;

      for (;;)
      {
        const HRESULT res = Buff->Lock(Cursor, srcSize,
          &part1Data, &part1Size, &part2Data, &part2Size, 0);
        if (DSERR_BUFFERLOST == res)
        {
          Log::Debug(THIS_MODULE, "Buffer lost. Retry to lock");
          Buff->Restore();
          continue;
        }
        CheckWin32Error(res, THIS_LINE);
        break;
      }
      assert(srcSize == part1Size + part2Size);
      const uint8_t* const src = safe_ptr_cast<const uint8_t*>(&buffer[0]);
      std::memcpy(part1Data, src, part1Size);
      if (part2Data)
      {
        std::memcpy(part2Data, src + part1Size, part2Size);
      }
      Cursor += srcSize;
      Cursor %= BuffSize;
      CheckWin32Error(Buff->Unlock(part1Data, part1Size, part2Data, part2Size), THIS_LINE);
    }

    void Pause()
    {
      LPVOID part1Data = 0, part2Data = 0;
      DWORD part1Size = 0, part2Size = 0;
      for (;;)
      {
        const HRESULT res = Buff->Lock(0, 0,
          &part1Data, &part1Size, &part2Data, &part2Size, DSBLOCK_ENTIREBUFFER);
        if (DSERR_BUFFERLOST == res)
        {
          Log::Debug(THIS_MODULE, "Buffer lost. Retry to lock");
          Buff->Restore();
          continue;
        }
        CheckWin32Error(res, THIS_LINE);
        break;
      }
      std::memset(part1Data, 0, part1Size);
      if (part2Data)
      {
        std::memset(part2Data, 0, part2Size);
      }
      CheckWin32Error(Buff->Unlock(part1Data, part1Size, part2Data, part2Size), THIS_LINE);
    }

    void Stop()
    {
      CheckWin32Error(Buff->Stop(), THIS_LINE);
      while (IsPlaying())
      {
        Wait();
      }
    }

    DirectSoundBufferPtr GetBuffer() const
    {
      return Buff;
    }
  private:
    bool WaitForFree(std::size_t srcSize) const
    {
      while (GetBytesToWrite() < srcSize)
      {
        if (!IsPlaying())
        {
          return false;
        }
        Wait();
      }
      return true;
    }

    std::size_t GetBytesToWrite() const
    {
      DWORD playCursor = 0;
      CheckWin32Error(Buff->GetCurrentPosition(&playCursor, NULL), THIS_LINE);
      if (Cursor > playCursor) //go ahead
      {
        return BuffSize - (Cursor - playCursor);
      }
      else //overlap
      {
        return playCursor - Cursor;
      }
    }

    bool IsPlaying() const
    {
      DWORD status = 0;
      CheckWin32Error(Buff->GetStatus(&status), THIS_LINE);
      return 0 != (status & DSBSTATUS_PLAYING);
    }

    void Wait() const
    {
      boost::this_thread::sleep(SleepPeriod);
    }
  private:
    const DirectSoundBufferPtr Buff;
    const boost::posix_time::millisec SleepPeriod;
    std::size_t BuffSize;
    std::size_t Cursor;
  };

  class DirectSoundVolumeControl : public VolumeControl
  {
  public:
    explicit DirectSoundVolumeControl(const StreamBuffer::Ptr& stream)
      : Stream(stream)
    {
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      if (!Stream)
      {
        volume = MultiGain();
        return Error();
      }
      try
      {
        Log::Debug(THIS_MODULE, "GetVolume");
        const DirectSoundBufferPtr buf = Stream->GetBuffer();
        LONG pan = 0, vol = 0;
        CheckWin32Error(buf->GetPan(&pan), THIS_LINE);
        CheckWin32Error(buf->GetVolume(&vol), THIS_LINE);
        BOOST_STATIC_ASSERT(OUTPUT_CHANNELS == 2);
        //in hundredths of a decibel
        const double attLeftDb = double(vol - (pan > 0 ? pan : 0)) / 100;
        const double attRightDb = double(vol - (pan < 0 ? -pan : 0)) / 100;
        //attenuation = 20 log10 volume, vol = 10^^(att/20)
        volume[0] = std::pow(10.0, attLeftDb / 20);
        volume[1] = std::pow(10.0, attRightDb / 20);
        return Error();
      }
      catch (const Error& err)
      {
        return err;
      }
    }

    virtual Error SetVolume(const MultiGain& volume)
    {
      if (volume.end() != std::find_if(volume.begin(), volume.end(), std::bind2nd(std::greater<Gain>(), Gain(1.0))))
      {
        return Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_GAIN);
      }
      if (!Stream)
      {
        return Error();
      }
      try
      {
        Log::Debug(THIS_MODULE, "SetVolume");
        const DirectSoundBufferPtr buf = Stream->GetBuffer();
        const int_t volLeft = static_cast<uint_t>(std::log10(volume[0]) * 20 * 100);
        const int_t volRight = static_cast<uint_t>(std::log10(volume[1]) * 20 * 100);
        const LONG vol = std::max(volLeft, volRight);
        //pan is negative for left
        const LONG pan = volLeft < vol ? vol - volLeft : vol - volRight;
        CheckWin32Error(buf->SetVolume(vol), THIS_LINE);
        CheckWin32Error(buf->SetPan(pan), THIS_LINE);
        return Error();
      }
      catch (const Error& err)
      {
        return err;
      }
    }
  private:
    const StreamBuffer::Ptr& Stream;
  };

  class DirectSoundBackendParameters
  {
  public:
    explicit DirectSoundBackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    std::size_t GetLatency() const
    {
      Parameters::IntType latency = Parameters::ZXTune::Sound::Backends::DirectSound::LATENCY_DEFAULT;
      if (Accessor.FindIntValue(Parameters::ZXTune::Sound::Backends::DirectSound::LATENCY, latency) &&
          !in_range<Parameters::IntType>(latency, LATENCY_MIN, LATENCY_MAX))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_DSOUND_BACKEND_INVALID_LATENCY, static_cast<int_t>(latency), LATENCY_MIN, LATENCY_MAX);
      }
      return static_cast<std::size_t>(latency);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class DirectSoundBackendWorker : public BackendWorker
  {
  public:
    explicit DirectSoundBackendWorker(Parameters::Accessor::Ptr params)
      : BackendParams(params)
      , RenderingParameters(RenderParameters::Create(BackendParams))
    {
    }

    virtual void Test()
    {
      OpenDeviceAndBuffer();
    }

    virtual void OnStartup(const Module::Holder& /*module*/)
    {
      Log::Debug(THIS_MODULE, "Starting");
      const std::pair<DirectSoundDevicePtr, StreamBuffer::Ptr> result = OpenDeviceAndBuffer();
      Stream = result.second;
      Device = result.first;
      Log::Debug(THIS_MODULE, "Started");
    }

    virtual void OnShutdown()
    {
      Log::Debug(THIS_MODULE, "Stopping");
      Stream->Stop();
      Stream.reset();
      Device = DirectSoundDevicePtr();
      Log::Debug(THIS_MODULE, "Stopped");
    }

    virtual void OnPause()
    {
      Stream->Pause();
    }

    virtual void OnResume()
    {
    }

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      Stream->Add(buffer);
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      return boost::make_shared<DirectSoundVolumeControl>(Stream);
    }
  private:
    std::pair<DirectSoundDevicePtr, StreamBuffer::Ptr> OpenDeviceAndBuffer()
    {
      const DirectSoundBackendParameters params(*BackendParams);
      const DirectSoundDevicePtr device = OpenDevice();
      const uint_t latency = params.GetLatency();
      const DirectSoundBufferPtr buffer = CreateBuffer(device, RenderingParameters->SoundFreq(), latency);
      const uint_t frameDurationMs = RenderingParameters->FrameDurationMicrosec() / 1000;
      const StreamBuffer::Ptr stream = boost::make_shared<StreamBuffer>(buffer, boost::posix_time::millisec(frameDurationMs));
      return std::make_pair(device, stream);
    }
  private:
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    DirectSoundDevicePtr Device;
    StreamBuffer::Ptr Stream;
  };

  class DirectSoundBackendCreator : public BackendCreator
  {
  public:
    virtual String Id() const
    {
      return DSOUND_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::DSOUND_BACKEND_DESCRIPTION;
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
        const BackendWorker::Ptr worker(new DirectSoundBackendWorker(allParams));
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
  };

  BOOL CALLBACK EnumerateDevicesCallback(LPGUID /*guid*/, LPCSTR descr, LPCSTR /*module*/, LPVOID param)
  {
    Log::Debug(THIS_MODULE, "Detected device '%1%'", descr);
    uint_t& devices = *safe_ptr_cast<uint_t*>(param);
    ++devices;
    return TRUE;
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterDirectSoundBackend(BackendsEnumerator& enumerator)
    {
      if (DirectSoundLibrary::Instance().IsAccessible())
      {
        uint_t devices = 0;
        if (DS_OK != ::DirectSoundEnumerate(&EnumerateDevicesCallback, &devices))
        {
          Log::Debug(THIS_MODULE, "Failed to enumerate devices. Skip backend.");
          return;
        }
        if (0 == devices)
        {
          Log::Debug(THIS_MODULE, "No devices to output. Skip backend");
          return;
        }
        Log::Debug(THIS_MODULE, "Detected %1% devices to output.", devices);
        const BackendCreator::Ptr creator(new DirectSoundBackendCreator());
        enumerator.RegisterCreator(creator);
      }
    }
  }
}

//global namespace
#define STR(a) #a
//MSVS2003 does not support variadic macros
#define DIRECTSOUND_CALL2(func, p1, p2) DirectSoundLibrary::Instance().GetSymbol(&func, STR(func))(p1, p2)
#define DIRECTSOUND_CALL3(func, p1, p2, p3) DirectSoundLibrary::Instance().GetSymbol(&func, STR(func))(p1, p2, p3)

HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA cb, LPVOID param)
{
  return DIRECTSOUND_CALL2(DirectSoundEnumerateA, cb, param);
}

//MSVS has different prototype comparing to mingw one
#ifdef _MSC_VER
HRESULT WINAPI DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
#else
HRESULT WINAPI DirectSoundCreate(LPGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
#endif
{
  return DIRECTSOUND_CALL3(DirectSoundCreate, pcGuidDevice, ppDS, pUnkOuter);
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterDirectSoundBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}

#endif
