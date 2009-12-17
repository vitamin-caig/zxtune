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
#include "../error_codes.h"

#include <boost/noncopyable.hpp>

#include <windows.h>

#include <algorithm>

#include <text/backends.h>

#define FILE_TAG 5E3F141A

namespace
{
  using namespace ZXTune::Sound;

  const Char BACKEND_ID[] = {'w', 'i', 'n', '3', '2', 0};
  const String BACKEND_VERSION(FromChar("$Rev$"));

  static const Backend::Info BACKEND_INFO =
  {
    BACKEND_ID,
    BACKEND_VERSION,
    TEXT_WIN32_BACKEND_DESCRIPTION
  };

  void CheckMMResult(::MMRESULT res)
  {
    if (MMSYSERR_NOERROR != res)
    {
      throw MakeFormattedError(THIS_LINE, BACKEND_PLATFORM_ERROR, TEXT_SOUND_ERROR_WIN32_BACKEND_ERROR, res);//TODO
    }
  }

  class WaveBuffer
  {
  public:
    WaveBuffer()
      : Header(), Buffer()
    {
    }

    void Allocate(::HWAVEOUT handle, const RenderParameters& params)
    {
      const std::size_t bufSize(params.SamplesPerFrame());
      Buffer.resize(bufSize);
      Header.lpData = ::LPSTR(&Buffer[0]);
      Header.dwBufferLength = ::DWORD(bufSize) * sizeof(Buffer.front());
      Header.dwUser = Header.dwLoops = Header.dwFlags = 0;
      CheckMMResult(::waveOutPrepareHeader(handle, &Header, sizeof(Header)));
      Header.dwFlags |= WHDR_DONE;
    }

    void Release(::HWAVEOUT handle, ::HANDLE event)
    {
      while (!(Header.dwFlags & WHDR_DONE))
      {
        ::WaitForSingleObject(event, INFINITE);
      }
      CheckMMResult(::waveOutUnprepareHeader(handle, &Header, sizeof(Header)));
      std::vector<MultiSample>().swap(Buffer);
    }


    void Fill(::HANDLE event, const std::vector<MultiSample>& buf)
    {
      while (!(Header.dwFlags & WHDR_DONE))
      {
        ::WaitForSingleObject(event, INFINITE);
      }
      assert(buf.size() <= Buffer.size());
      Header.dwBufferLength = static_cast< ::DWORD>(std::min<std::size_t>(buf.size(), Buffer.size())) * sizeof(Buffer.front());
      std::memcpy(Header.lpData, &buf[0], Header.dwBufferLength);
    }

    void Play(::HWAVEOUT handle)
    {
      Header.dwFlags &= ~WHDR_DONE;
      CheckMMResult(::waveOutWrite(handle, &Header, sizeof(Header)));
    }
  private:
    ::WAVEHDR Header;
    std::vector<MultiSample> Buffer;
  };

  class Win32Backend : public BackendImpl, private boost::noncopyable
  {
  public:
    Win32Backend()
      : WaveHandle(0), Device(WAVE_MAPPER), Event(::CreateEvent(0, FALSE, FALSE, 0))
      , PlayBuffer(new WaveBuffer()), FillBuffer(new WaveBuffer())
    {
    }

    virtual ~Win32Backend()
    {
      assert(0 == WaveHandle || !"Win32Backend::Stop should be called before exit");
      ::CloseHandle(Event);
    }

    virtual void GetInfo(Info& info) const
    {
      info = BACKEND_INFO;
    }

    virtual Error GetVolume(MultiGain& /*volume*/) const
    {
      return Error();//TODO
    }

    virtual Error SetVolume(const MultiGain& /*volume*/)
    {
      return Error();//TODO
    }

    virtual void OnStartup()
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

      CheckMMResult(::waveOutOpen(&WaveHandle, Device, &wfx, DWORD_PTR(Event), 0,
        CALLBACK_EVENT | WAVE_FORMAT_DIRECT));
      PlayBuffer->Allocate(WaveHandle, RenderingParameters);
      FillBuffer->Allocate(WaveHandle, RenderingParameters);
      ::ResetEvent(Event);
    }

    virtual void OnShutdown()
    {
      ::ResetEvent(Event);
      PlayBuffer->Release(WaveHandle, Event);
      FillBuffer->Release(WaveHandle, Event);
      if (0 != WaveHandle)
      {
        CheckMMResult(::waveOutReset(WaveHandle));
        CheckMMResult(::waveOutClose(WaveHandle));
        WaveHandle = 0;
      }
    }

    virtual void OnPause()
    {
      if (0 != WaveHandle)
      {
        CheckMMResult(::waveOutPause(WaveHandle));
      }
    }

    virtual void OnResume()
    {
      if (0 != WaveHandle)
      {
        CheckMMResult(::waveOutRestart(WaveHandle));
      }
    }

    virtual void OnParametersChanged(const ParametersMap& /*updates*/)
    {
      /*
      const unsigned mask(DRIVER_PARAMS | DRIVER_FLAGS | BUFFER | SOUND_FREQ);
      if (changedFields & mask)
      {
        const bool needStartup(0 != WaveHandle);
        OnShutdown();

        //request devices
        while (changedFields & DRIVER_PARAMS)
        {
          if (Params.DriverParameters.empty())
          {
            Device = WAVE_MAPPER;
            break;
          }
          InStringStream str(Params.DriverParameters);
          unsigned id(0);
          if (!(str >> id))
          {
            assert(!"Invalid parameter format");
            break;
          }
          const ::UINT devs(::waveOutGetNumDevs());
          if (devs <= id)
          {
            assert(!"Invalid device specified");
            break;
          }
          Device = id;
          break;
        }

        //check
        Parent::OnParametersChanged(changedFields);
        if (needStartup)
        {
          OnStartup();
        }
      }
      */
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      FillBuffer->Fill(Event, buffer);
      FillBuffer->Play(WaveHandle);
      PlayBuffer.swap(FillBuffer);
    }
  private:
    ::HWAVEOUT WaveHandle;
    ::UINT Device;
    ::HANDLE Event;
    boost::scoped_ptr<WaveBuffer> PlayBuffer;
    boost::scoped_ptr<WaveBuffer> FillBuffer;
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
      enumerator.RegisterBackend(BACKEND_INFO, Win32BackendCreator);
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
