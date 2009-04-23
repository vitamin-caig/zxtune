#include "../sound_backend_impl.h"

#include <windows.h>

#include <tools.h>

#include <cassert>
#include <algorithm>

namespace
{
  using namespace ZXTune::Sound;

  const std::size_t MIN_BUFFER_DEPTH = 3;
  const std::size_t MAX_BUFFER_DEPTH = 4;

  class Event
  {
  public:
    Event() : Handle(::CreateEvent(0, TRUE, TRUE, 0))
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

    void Wait()
    {
      ::WaitForSingleObject(Handle, 10000);//TODO
      ::ResetEvent(Handle);
    }

  private:
    ::HANDLE Handle;
  };

  class BufferDescr
  {
    BufferDescr& operator = (BufferDescr&);
  public:
    explicit BufferDescr(const ::HWAVEOUT& handle) : Handle(handle)
    {
      std::memset(&Header, 0, sizeof(Header));
    }
    ~BufferDescr()
    {
      Unprepare();
    }

    //copy only handler
    BufferDescr(const BufferDescr& oth) : Handle(oth.Handle)
    {
      std::memset(&Header, 0, sizeof(Header));
    }

    void Fill(const void* data, std::size_t sizeInSamples)
    {
      Unprepare();
      Header.lpData = ::LPSTR(&Buffer[0]);
      Header.dwBufferLength = ::DWORD(sizeInSamples) * sizeof(Buffer.front());
      std::memcpy(Header.lpData, data, Header.dwBufferLength);
      assert(Handle);
      ::waveOutPrepareHeader(Handle, &Header, sizeof(Header));
    }

    void Play()
    {
      assert(Handle);
      if (Header.lpData && MMSYSERR_NOERROR != ::waveOutWrite(Handle, &Header, sizeof(Header)))
      {
        throw 1;//TODO
      }
    }

    bool Ready()
    {
      return Header.lpData && 0 != (Header.dwFlags & WHDR_DONE);
    }

    void Stop()
    {
      Unprepare();
    }

    void Resize(std::size_t sizeInSamples)
    {
      assert(0 == Header.lpData);
      if (sizeInSamples > Buffer.size())
      {
        Buffer.resize(sizeInSamples);
      }
    }
  private:
    void Unprepare()
    {
      if (Header.lpData)
      {
        assert(Handle);
        ::waveOutUnprepareHeader(Handle, &Header, sizeof(Header));
        Header.lpData = 0;
      }
    }
  private:
    const ::HWAVEOUT& Handle;
    ::WAVEHDR Header;
    std::vector<Sample> Buffer;
  };

  class Win32Backend : public BackendImpl
  {
  public:
    Win32Backend()
      : WaveHandle(0), Buffers(MAX_BUFFER_DEPTH, BufferDescr(WaveHandle))
      , FillPtr(&Buffers[0], &Buffers[MIN_BUFFER_DEPTH])
      , PlayPtr(FillPtr)
    {
    }

    virtual void OnBufferReady(const void* data, std::size_t sizeInBytes)
    {
      assert(WaveHandle);
      ++FillPtr;
      while (FillPtr == PlayPtr)
      {
        PlayedEvent.Wait();
      }
      FillPtr->Fill(data, sizeInBytes / sizeof(Sample));
      FilledEvent.Set();
    }

    virtual void OnParametersChanged(unsigned changedFields)
    {
      const bool freqChanged(0 != (changedFields & SOUND_FREQ));
      const bool bufChanged(0 != (changedFields & BUFFER));
      if (freqChanged || bufChanged)
      {
        Shutdown();
        if (changedFields & DRIVER_FLAGS)
        {
          PlayPtr = FillPtr = cycled_iterator<BufferDescr*>(&Buffers[0], &Buffers[
            clamp<std::size_t>(Params.DriverFlags, MIN_BUFFER_DEPTH, MAX_BUFFER_DEPTH)]);
        }

        if (bufChanged)
        {
          std::for_each(Buffers.begin(), Buffers.end(), std::bind2nd(std::mem_fun_ref(&BufferDescr::Resize),
            OUTPUT_CHANNELS * (Params.SoundParameters.SoundFreq * Params.BufferInMs / 1000)));
        }

        ::WAVEFORMATEX wfx;

        std::memset(&wfx, 0, sizeof(wfx));
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = OUTPUT_CHANNELS;
        wfx.nSamplesPerSec = Params.SoundParameters.SoundFreq;
        wfx.nBlockAlign = sizeof(SampleArray);
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        wfx.wBitsPerSample = 8 * sizeof(Sample);

        if (MMSYSERR_NOERROR != 
          ::waveOutOpen(&WaveHandle, WAVE_MAPPER, &wfx, ::DWORD_PTR(ReadyEvent.GetHandle()), 0,
                CALLBACK_EVENT | WAVE_FORMAT_DIRECT))
        {
          throw 3;//TODO
        }

        PlaybackThread.Start(PlaybackFunc, this);
      }
    }

    virtual void OnShutdown()
    {
      Shutdown();
    }
  private:
    void Shutdown()
    {
      std::for_each(Buffers.begin(), Buffers.end(), std::mem_fun_ref(&BufferDescr::Stop));
      if (0 != WaveHandle)
      {
        ::waveOutClose(WaveHandle);
        WaveHandle = 0;
      }
    }

    static bool PlaybackFunc(void* data)
    {
      Win32Backend* const self(static_cast<Win32Backend*>(data));
      if (0 == self->WaveHandle)
      {
        return false;
      }
      while (self->FillPtr == self->PlayPtr)
      {
        self->FilledEvent.Wait();
      }
      self->PlayPtr->Play();
      self->ReadyEvent.Wait();
      ++self->PlayPtr;
      self->PlayedEvent.Set();
      return true;
    }
  private:
    ::HWAVEOUT WaveHandle;
    Event ReadyEvent;
    ZXTune::IPC::Thread PlaybackThread;
    Event FilledEvent;
    Event PlayedEvent;
    std::vector<BufferDescr> Buffers;
    cycled_iterator<BufferDescr*> FillPtr;
    cycled_iterator<BufferDescr*> PlayPtr;
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
