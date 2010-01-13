/*
Abstract:
  Win32 waveout backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef WIN32_WAVEOUT_SUPPORT

#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"

#include <tools.h>
#include <error_tools.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>

#include <boost/noncopyable.hpp>

//platform-dependent
#include <windows.h>

#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include <algorithm>

#include <text/backends.h>
#include <text/sound.h>

#define FILE_TAG 5E3F141A

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const unsigned MAX_WIN32_VOLUME = 0xffff;

  const Char BACKEND_ID[] = {'w', 'i', 'n', '3', '2', 0};
  const String BACKEND_VERSION(FromChar("$Rev$"));

  static const BackendInfo BACKEND_INFO =
  {
    BACKEND_ID,
    TEXT_WIN32_BACKEND_DESCRIPTION,
    BACKEND_VERSION
  };

  inline void CheckMMResult(::MMRESULT res, Error::LocationRef loc)
  {
    if (MMSYSERR_NOERROR != res)
    {
      std::vector<char> buffer(1024);
      if (MMSYSERR_NOERROR == ::waveOutGetErrorText(res, &buffer[0], buffer.size()))
      {
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, TEXT_SOUND_ERROR_WIN32_BACKEND_ERROR, 
          String(buffer.begin(), std::find(buffer.begin(), buffer.end(), '\0')));
      }
      else
      {
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, TEXT_SOUND_ERROR_WIN32_BACKEND_ERROR, res);
      }
    }
  }

  inline void CheckPlatformResult(bool val, Error::LocationRef loc)
  {
    if (!val)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, TEXT_SOUND_ERROR_WIN32_BACKEND_ERROR, ::GetLastError());//TODO
    }
  }

  class WaveBuffer
  {
  public:
    WaveBuffer()
      : Header(), Buffer(), Handle(0), Event(INVALID_HANDLE_VALUE)
    {
    }

    ~WaveBuffer()
    {
      assert(0 == Handle);
    }

    void Allocate(::HWAVEOUT handle, ::HANDLE event, const RenderParameters& params)
    {
      assert(0 == Handle && INVALID_HANDLE_VALUE == Event);
      const std::size_t bufSize(params.SamplesPerFrame());
      Buffer.resize(bufSize);
      Header.lpData = ::LPSTR(&Buffer[0]);
      Header.dwBufferLength = ::DWORD(bufSize) * sizeof(Buffer.front());
      Header.dwUser = Header.dwLoops = Header.dwFlags = 0;
      CheckMMResult(::waveOutPrepareHeader(handle, &Header, sizeof(Header)), THIS_LINE);
      Header.dwFlags |= WHDR_DONE;
      Handle = handle;
      Event = event;
    }

    void Release()
    {
      Wait();
      CheckMMResult(::waveOutUnprepareHeader(Handle, &Header, sizeof(Header)), THIS_LINE);
      std::vector<MultiSample>().swap(Buffer);
      Handle = 0;
      Event = INVALID_HANDLE_VALUE;
    }

    void Process(const std::vector<MultiSample>& buf)
    {
      Wait();
      assert(Header.dwFlags & WHDR_DONE);
      assert(buf.size() <= Buffer.size());
      Header.dwBufferLength = static_cast< ::DWORD>(std::min<std::size_t>(buf.size(), Buffer.size())) * sizeof(Buffer.front());
      std::memcpy(Header.lpData, &buf[0], Header.dwBufferLength);
      Header.dwFlags &= ~WHDR_DONE;
      CheckMMResult(::waveOutWrite(Handle, &Header, sizeof(Header)), THIS_LINE);
    }
  private:
    void Wait()
    {
      while (!(Header.dwFlags & WHDR_DONE))
      {
        CheckPlatformResult(WAIT_OBJECT_0 == ::WaitForSingleObject(Event, INFINITE), THIS_LINE);
      }
    }
  private:
    ::WAVEHDR Header;
    std::vector<MultiSample> Buffer;
    ::HWAVEOUT Handle;
    ::HANDLE Event;
  };

  class Win32Backend : public BackendImpl, private boost::noncopyable
  {
  public:
    Win32Backend()
      : CurrentBuffer(Buffers.begin(), Buffers.end())
      , Event(::CreateEvent(0, FALSE, FALSE, 0))
      //device identifier used for opening and volume control
      , Device(Parameters::ZXTune::Sound::Backends::Win32::DEVICE_DEFAULT)
      , WaveHandle(0)
    {
    }

    virtual ~Win32Backend()
    {
      assert(0 == WaveHandle || !"Win32Backend::Stop should be called before exit");
      ::CloseHandle(Event);
    }

    virtual void GetInfo(BackendInfo& info) const
    {
      info = BACKEND_INFO;
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      try
      {
        Locker lock(BackendMutex);
        boost::array<uint16_t, OUTPUT_CHANNELS> buffer;
        BOOST_STATIC_ASSERT(sizeof(buffer) == sizeof(DWORD));
        CheckMMResult(::waveOutGetVolume(reinterpret_cast< ::HWAVEOUT>(Device), safe_ptr_cast<LPDWORD>(&buffer[0])), THIS_LINE);
        std::transform(buffer.begin(), buffer.end(), volume.begin(), std::bind2nd(std::divides<Gain>(), MAX_WIN32_VOLUME));
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
        boost::array<uint16_t, OUTPUT_CHANNELS> buffer;
        std::transform(volume.begin(), volume.end(), buffer.begin(), std::bind2nd(std::multiplies<Gain>(), Gain(MAX_WIN32_VOLUME)));
        BOOST_STATIC_ASSERT(sizeof(buffer) == sizeof(DWORD));
        CheckMMResult(::waveOutSetVolume(reinterpret_cast< ::HWAVEOUT>(Device), *safe_ptr_cast<LPDWORD>(&buffer[0])), THIS_LINE);
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
      Locker lock(BackendMutex);
      if (0 != WaveHandle)
      {
        CheckMMResult(::waveOutPause(WaveHandle), THIS_LINE);
      }
    }

    virtual void OnResume()
    {
      Locker lock(BackendMutex);
      if (0 != WaveHandle)
      {
        CheckMMResult(::waveOutRestart(WaveHandle), THIS_LINE);
      }
    }

    virtual void OnParametersChanged(const Parameters::Map& updates)
    {
      const Parameters::IntType* const device =
        Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::Backends::Win32::DEVICE);
      const Parameters::IntType* const freq =
        Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::FREQUENCY);
      if (device || freq)
      {
        Locker lock(BackendMutex);
        const bool needStartup(0 != WaveHandle);
        DoShutdown();
        Device = *device;
        if (needStartup)
        {
          DoStartup();
        }
      }
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      Locker lock(BackendMutex);
      CurrentBuffer->Process(buffer);
      ++CurrentBuffer;
    }
  private:
    void DoStartup()
    {
      assert(0 == WaveHandle);
      ::WAVEFORMATEX wfx;

      std::memset(&wfx, 0, sizeof(wfx));
      wfx.wFormatTag = WAVE_FORMAT_PCM;
      wfx.nChannels = OUTPUT_CHANNELS;
      wfx.nSamplesPerSec = RenderingParameters.SoundFreq;
      wfx.nBlockAlign = sizeof(MultiSample);
      wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
      wfx.wBitsPerSample = 8 * sizeof(Sample);

      CheckMMResult(::waveOutOpen(&WaveHandle, static_cast< ::UINT>(Device), &wfx, DWORD_PTR(Event), 0,
        CALLBACK_EVENT | WAVE_FORMAT_DIRECT), THIS_LINE);
      std::for_each(Buffers.begin(), Buffers.end(), boost::bind(&WaveBuffer::Allocate, _1, WaveHandle, Event, boost::cref(RenderingParameters)));
      CheckPlatformResult(0 != ::ResetEvent(Event), THIS_LINE);
    }

    void DoShutdown()
    {
      std::for_each(Buffers.begin(), Buffers.end(), boost::mem_fn(&WaveBuffer::Release));
      if (0 != WaveHandle)
      {
        CheckMMResult(::waveOutReset(WaveHandle), THIS_LINE);
        CheckMMResult(::waveOutClose(WaveHandle), THIS_LINE);
        WaveHandle = 0;
      }
    }
  private:
    //TODO: setup buffer depth
    boost::array<WaveBuffer, 3> Buffers;
    cycled_iterator<WaveBuffer*> CurrentBuffer;
    ::HANDLE Event;
    int64_t Device;
    ::HWAVEOUT WaveHandle;
  };

  Backend::Ptr Win32BackendCreator()
  {
    return Backend::Ptr(new SafeBackendWrapper<Win32Backend>());
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterWin32Backend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterBackend(BACKEND_INFO, Win32BackendCreator, BACKEND_PRIORITY_HIGH);
    }
  }
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterWin32Backend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}

#endif
