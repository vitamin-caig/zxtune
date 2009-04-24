#include "../sound_backend_impl.h"

#include <windows.h>

#include <tools.h>

#include <cassert>
#include <algorithm>

namespace
{
  using namespace ZXTune::Sound;

  const std::size_t DEFAULT_BUFFER_DEPTH = 2;
  const std::size_t MIN_BUFFER_DEPTH = 2;
  const std::size_t MAX_BUFFER_DEPTH = 4;

  void CheckMMResult(::MMRESULT res)
  {
    if (MMSYSERR_NOERROR != res)
    {
      throw 1;//TODO
    }
  }

  class Event
  {
  public:
    Event() : Handle(::CreateEvent(0, TRUE, FALSE, 0))
    {
    }

    ~Event()
    {
      ::CloseHandle(Handle);
    }

    ::HANDLE GetHandle() const
    {
      return Handle;
    }

    void Set()
    {
      ::SetEvent(Handle);
    }

    void Reset()
    {
      ::ResetEvent(Handle);
    }

    bool Wait()
    {
      if (WAIT_OBJECT_0 == ::WaitForSingleObject(Handle, INFINITE))
      {
        Reset();
        return true;
      }
      return false;
    }

    bool IsSet()
    {
      return WAIT_OBJECT_0 == ::WaitForSingleObject(Handle, 0);
    }
  private:
    ::HANDLE Handle;
  };

  class BufferDescr
  {
  public:
    BufferDescr(const ::HWAVEOUT& handle, std::size_t sizeInSamples) : Handle(handle), Buffer(sizeInSamples)
    {
      Init();
    }

    BufferDescr(const BufferDescr& rh) : Handle(rh.Handle), Buffer(rh.Buffer.size())
    {
      Init();
    }

    const BufferDescr& operator = (const BufferDescr&)
    {
      assert(false);
      return *this;
    }

    ~BufferDescr()
    {
      try
      {
        assert(0 != Handle);
        CheckMMResult(::waveOutUnprepareHeader(Handle, &Header, sizeof(Header)));
      }
      catch (...)
      {
        //TODO
      }
    }

    void Fill(const void* data, std::size_t sizeInSamples)
    {
      assert(IsReady());
      assert(sizeInSamples <= Buffer.size());
      Header.dwBufferLength = ::DWORD(sizeInSamples) * sizeof(Buffer.front());
      std::memcpy(Header.lpData, data, Header.dwBufferLength);
    }

    void Play()
    {
      assert(0 != Handle);
      Header.dwFlags &= ~WHDR_DONE;
      CheckMMResult(::waveOutWrite(Handle, &Header, sizeof(Header)));
    }

    bool IsReady() const
    {
      return 0 != (Header.dwFlags & WHDR_DONE);
    }

  private:
    void Init()
    {
      Header.lpData = ::LPSTR(&Buffer[0]);
      Header.dwBufferLength = ::DWORD(Buffer.size()) * sizeof(Buffer.front());
      Header.dwUser = Header.dwLoops = Header.dwFlags = 0;
      assert(0 != Handle);
      CheckMMResult(::waveOutPrepareHeader(Handle, &Header, sizeof(Header)));
      Header.dwFlags |= WHDR_DONE;
    }
  private:
    const ::HWAVEOUT& Handle;
    std::vector<Sample> Buffer;
    ::WAVEHDR Header;
  };

  class Win32Backend : public BackendImpl
  {
  public:
    Win32Backend()
      : WaveHandle(0), Device(WAVE_MAPPER)
    {
    }

    virtual ~Win32Backend()
    {
      try
      {
        Close();
      }
      catch (...)
      {
        //TODO
      }
    }

    virtual void OnBufferReady(const void* data, std::size_t sizeInBytes)
    {
      while (!FillPtr->IsReady())
      {
        ReadyEvent.Wait();
      }
      FillPtr->Fill(data, sizeInBytes / sizeof(Sample));
      FillPtr->Play();
      ++FillPtr;
    }

    virtual void OnParametersChanged(unsigned changedFields)
    {
      const unsigned mask(DRIVER_PARAMS | DRIVER_FLAGS | BUFFER | SOUND_FREQ | PREAMP);

      if (changedFields & mask)
      {
        Close();

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

        ::WAVEFORMATEX wfx;

        std::memset(&wfx, 0, sizeof(wfx));
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = OUTPUT_CHANNELS;
        wfx.nSamplesPerSec = Params.SoundParameters.SoundFreq;
        wfx.nBlockAlign = sizeof(SampleArray);
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        wfx.wBitsPerSample = 8 * sizeof(Sample);

        CheckMMResult(::waveOutOpen(&WaveHandle, Device, &wfx, ::DWORD_PTR(ReadyEvent.GetHandle()), 0,
                CALLBACK_EVENT | WAVE_FORMAT_DIRECT));

        if (changedFields & PREAMP)
        {
          ::DWORD vol(0xffff * Params.Preamp / FIXED_POINT_PRECISION);
          CheckMMResult(::waveOutSetVolume(WaveHandle, vol | (vol << 16)));
        }

        if (changedFields & (BUFFER | SOUND_FREQ | DRIVER_FLAGS))
        {
          unsigned buffers(DEFAULT_BUFFER_DEPTH);
          if (changedFields & DRIVER_FLAGS)
          {
            if (Params.DriverFlags < MIN_BUFFER_DEPTH || Params.DriverFlags > MAX_BUFFER_DEPTH)
            {
              throw 2;//TODO
            }
            buffers = Params.DriverFlags;
          }
          Buffers.assign(buffers, 
            BufferDescr(WaveHandle, OUTPUT_CHANNELS * (Params.SoundParameters.SoundFreq * Params.BufferInMs / 1000)));
          FillPtr = cycled_iterator<BufferDescr*>(&Buffers[0], &Buffers[Buffers.size()]);
        }
      }
    }

    virtual void OnShutdown()
    {
      if (0 != WaveHandle)
      {
        CheckMMResult(::waveOutReset(WaveHandle));
      }
    }
  private:
    void Close()
    {
      if (0 != WaveHandle)
      {
        Buffers.clear();
        CheckMMResult(::waveOutClose(WaveHandle));
        WaveHandle = 0;
      }
    }
  private:
    ::HWAVEOUT WaveHandle;
    ::UINT Device;
    Event ReadyEvent;
    std::vector<BufferDescr> Buffers;
    cycled_iterator<BufferDescr*> FillPtr;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Backend::Ptr CreateWinAPIBackend()
    {
      return Backend::Ptr(new Win32Backend);
    }
  }
}
