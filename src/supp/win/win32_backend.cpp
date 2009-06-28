#include "../backend_enumerator.h"
#include "../sound_backend_async.h"

#include <windows.h>

#include <tools.h>

#include <cassert>
#include <algorithm>

#include <text/errors.h>
#include <text/backends.h>

#define FILE_TAG 5E3F141A

namespace
{
  using namespace ZXTune::Sound;

  void CheckMMResult(::MMRESULT res)
  {
    if (MMSYSERR_NOERROR != res)
    {
      throw Error(ERROR_DETAIL, 1, (Formatter(TEXT_ERROR_BACKEND_INIT_WIN32) % res).str());//TODO
    }
  }

  struct WaveBuffer
  {
    typedef std::vector<Sample> BufType;
    BufType Buffer;
    mutable ::WAVEHDR Header;
  };

  void Descriptor(Backend::Info& info);

  class Win32Backend : public AsyncBackend<WaveBuffer>
  {
    typedef AsyncBackend<WaveBuffer> Parent;
  public:
    Win32Backend()
      : WaveHandle(0), Device(WAVE_MAPPER), Event(::CreateEvent(0, FALSE, FALSE, 0))
    {
    }

    virtual ~Win32Backend()
    {
      assert(0 == WaveHandle || !"Win32Backend::Stop should be called before exit");
      ::CloseHandle(Event);
    }

    virtual void GetInfo(Info& info) const
    {
      return Descriptor(info);
    }

    virtual void OnParametersChanged(unsigned changedFields)
    {
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
    }

    virtual void OnStartup()
    {
      assert(0 == WaveHandle);
      ::WAVEFORMATEX wfx;

      std::memset(&wfx, 0, sizeof(wfx));
      wfx.wFormatTag = WAVE_FORMAT_PCM;
      wfx.nChannels = OUTPUT_CHANNELS;
      wfx.nSamplesPerSec = Params.SoundParameters.SoundFreq;
      wfx.nBlockAlign = sizeof(SampleArray);
      wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
      wfx.wBitsPerSample = 8 * sizeof(Sample);

      CheckMMResult(::waveOutOpen(&WaveHandle, Device, &wfx, DWORD_PTR(Event), 0,
        CALLBACK_EVENT | WAVE_FORMAT_DIRECT));
      ::ResetEvent(Event);
      Parent::OnStartup();
    }

    virtual void OnShutdown()
    {
      ::ResetEvent(Event);
      Parent::OnShutdown();
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

  protected:
    virtual void AllocateBuffer(WaveBuffer& buf)
    {
      const std::size_t bufSize(OUTPUT_CHANNELS * Params.BufferInMultisamples());
      buf.Buffer.resize(OUTPUT_CHANNELS * bufSize);
      buf.Header.lpData = ::LPSTR(&buf.Buffer[0]);
      buf.Header.dwBufferLength = ::DWORD(bufSize) * sizeof(buf.Buffer.front());
      buf.Header.dwUser = buf.Header.dwLoops = buf.Header.dwFlags = 0;
      assert(0 != WaveHandle);
      CheckMMResult(::waveOutPrepareHeader(WaveHandle, &buf.Header, sizeof(buf.Header)));
      buf.Header.dwFlags |= WHDR_DONE;
    }
    virtual void ReleaseBuffer(WaveBuffer& buf)
    {
      assert(0 != WaveHandle);
      CheckMMResult(::waveOutUnprepareHeader(WaveHandle, &buf.Header, sizeof(buf.Header)));
      //hot swap is not accepted by some compilers
      buf.Buffer.clear();
    }
    virtual void FillBuffer(const void* data, std::size_t sizeInSamples, WaveBuffer& buf)
    {
      assert(sizeInSamples <= buf.Buffer.size());
      while (!(buf.Header.dwFlags & WHDR_DONE))
      {
        ::WaitForSingleObject(Event, INFINITE);
      }
      buf.Header.dwBufferLength = ::DWORD(sizeInSamples) * sizeof(buf.Buffer.front());
      std::memcpy(buf.Header.lpData, data, buf.Header.dwBufferLength);
    }
    virtual void PlayBuffer(const WaveBuffer& buf)
    {
      assert(0 != WaveHandle);
      buf.Header.dwFlags &= ~WHDR_DONE;
      CheckMMResult(::waveOutWrite(WaveHandle, &buf.Header, sizeof(buf.Header)));
    }
  private:
    ::HWAVEOUT WaveHandle;
    ::UINT Device;
    ::HANDLE Event;
  };

  void Descriptor(Backend::Info& info)
  {
    info.Description = TEXT_WIN32_BACKEND_DESCRIPTION;
    info.Key = WIN32_BACKEND_KEY;
  }

  Backend::Ptr Creator()
  {
    return Backend::Ptr(new Win32Backend);
  }

  BackendAutoRegistrator registrator(Creator, Descriptor);
}
