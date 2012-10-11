/*
Abstract:
  DirectSound backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dsound.h"
#include "dsound_api.h"
#include "backend_impl.h"
#include "enumerator.h"
#include "volume_control.h"
//common includes
#include <debug_log.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
//text includes
#include <sound/text/backends.h>

#define FILE_TAG BCBCECCC

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Debug::Stream Dbg("Sound::Backend::DirectSound");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM | CAP_FEAT_HWVOLUME;

  const uint_t LATENCY_MIN = 20;
  const uint_t LATENCY_MAX = 10000;

  /*

   From http://msdn.microsoft.com/en-us/library/windows/desktop/dd797880(v=vs.85).aspx :

   For 8-bit PCM data, each sample is represented by a single unsigned data byte.
   For 16-bit PCM data, each sample is represented by a 16-bit signed value.
  */
  const bool SamplesShouldBeConverted = (sizeof(Sample) > 1) != SAMPLE_SIGNED;

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
      throw MakeFormattedError(loc, translate("Error in DirectSound backend: %1%."), res);
    }
  }

  void ReleaseRef(IUnknown* iface)
  {
    if (iface)
    {
      iface->Release();
    }
  }

  String Guid2String(LPGUID guid)
  {
    OLECHAR strGuid[39] = {0};
    if (!guid || 0 == ::StringFromGUID2(*guid, strGuid, ArraySize(strGuid)))
    {
      return String();
    }
    return String(strGuid, ArrayEnd(strGuid) - 1);
  }

  std::auto_ptr<GUID> String2Guid(const String& str)
  {
    if (str.empty())
    {
      return std::auto_ptr<GUID>();
    }
    std::vector<OLECHAR> strGuid(str.begin(), str.end());
    strGuid.push_back(0);
    std::auto_ptr<GUID> res(new GUID);
    CheckWin32Error(::CLSIDFromString(&strGuid[0], res.get()), THIS_LINE);
    return res;
  }

  typedef boost::shared_ptr<IDirectSound> DirectSoundDevicePtr;
  typedef boost::shared_ptr<IDirectSoundBuffer> DirectSoundBufferPtr;

  DirectSoundDevicePtr OpenDevice(DirectSound::Api& api, const String& device)
  {
    Dbg("OpenDevice(%1%)", device);
    DirectSoundDevicePtr::pointer raw = 0;
    const std::auto_ptr<GUID> deviceUuid = String2Guid(device);
    CheckWin32Error(api.DirectSoundCreate(deviceUuid.get(), &raw, NULL), THIS_LINE);
    const DirectSoundDevicePtr result = DirectSoundDevicePtr(raw, &ReleaseRef);
    CheckWin32Error(result->SetCooperativeLevel(GetWindowHandle(), DSSCL_PRIORITY), THIS_LINE);
    Dbg("Opened");
    return result;
  }

  DirectSoundBufferPtr CreateSecondaryBuffer(DirectSoundDevicePtr device, uint_t sampleRate, uint_t bufferInMs)
  {
    Dbg("CreateSecondaryBuffer");
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
    buffer.dwFlags = DSBCAPS_GLOBALFOCUS;
    buffer.dwBufferBytes = format.nAvgBytesPerSec * bufferInMs / 1000;
    buffer.lpwfxFormat = &format;

    LPDIRECTSOUNDBUFFER rawSecondary = 0;
    CheckWin32Error(device->CreateSoundBuffer(&buffer, &rawSecondary, NULL), THIS_LINE);
    assert(rawSecondary);
    const boost::shared_ptr<IDirectSoundBuffer> secondary(rawSecondary, &ReleaseRef);
    Dbg("Created");
    return secondary;
  }

  DirectSoundBufferPtr CreatePrimaryBuffer(DirectSoundDevicePtr device)
  {
    Dbg("CreatePrimaryBuffer");
    DSBUFFERDESC buffer;
    std::memset(&buffer, 0, sizeof(buffer));
    buffer.dwSize = sizeof(buffer);
    buffer.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;

    LPDIRECTSOUNDBUFFER rawPrimary = 0;
    CheckWin32Error(device->CreateSoundBuffer(&buffer, &rawPrimary, NULL), THIS_LINE);
    assert(rawPrimary);
    const boost::shared_ptr<IDirectSoundBuffer> primary(rawPrimary, &ReleaseRef);
    Dbg("Created");
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
      Dbg("Using cycle buffer size %1%", BuffSize);
      CheckWin32Error(Buff->Play(0, 0, DSBPLAY_LOOPING), THIS_LINE);
    }

    void Add(const Chunk& buffer)
    {
      std::size_t inputSize = buffer.size() * sizeof(buffer.front());
      const uint8_t* inputStart = safe_ptr_cast<const uint8_t*>(&buffer[0]);

      while (inputSize)
      {
        const std::size_t srcSize = std::min(inputSize, BuffSize / 2);
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
            Dbg("Buffer lost. Retry to lock");
            Buff->Restore();
            continue;
          }
          CheckWin32Error(res, THIS_LINE);
          break;
        }
        assert(srcSize == part1Size + part2Size);
        std::memcpy(part1Data, inputStart, part1Size);
        if (part2Data)
        {
          std::memcpy(part2Data, inputStart + part1Size, part2Size);
        }
        Cursor += srcSize;
        Cursor %= BuffSize;
        CheckWin32Error(Buff->Unlock(part1Data, part1Size, part2Data, part2Size), THIS_LINE);
        inputSize -= srcSize;
        inputStart += srcSize;
      }
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
          Dbg("Buffer lost. Retry to lock");
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

  //in centidecibell
  //use simple scale method due to less error in forward and backward conversion
  Gain AttenuationToGain(int_t cdB)
  {
    return 1.0 - Gain(cdB) / DSBVOLUME_MIN;
  }

  int_t GainToAttenuation(Gain level)
  {
    return static_cast<int_t>((1.0 - level) * DSBVOLUME_MIN);
  }

  class DirectSoundVolumeControl : public VolumeControl
  {
  public:
    explicit DirectSoundVolumeControl(DirectSoundBufferPtr buffer)
      : Buffer(buffer)
    {
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      try
      {
        const VolPan vols = GetVolume();
        BOOST_STATIC_ASSERT(OUTPUT_CHANNELS == 2);
        //in hundredths of a decibel
        const int_t attLeft = vols.first - (vols.second > 0 ? vols.second : 0);
        const int_t attRight = vols.first - (vols.second < 0 ? -vols.second : 0);
        volume[0] = AttenuationToGain(attLeft);
        volume[1] = AttenuationToGain(attRight);
        Dbg("GetVolume(vol=%1% pan=%2%) = {%3%, %4%}", 
          vols.first, vols.second, volume[0], volume[1]);
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
        return Error(THIS_LINE, translate("Failed to set volume: gain is out of range."));
      }
      try
      {
        const int_t attLeft = GainToAttenuation(volume[0]);
        const int_t attRight = GainToAttenuation(volume[1]);
        const LONG vol = std::max(attLeft, attRight);
        //pan is negative for left
        const LONG pan = attLeft < vol ? vol - attLeft : vol - attRight;
        Dbg("SetVolume(%1%, %2%) => vol=%3% pan=%4%", volume[0], volume[1], vol, pan);
        SetVolume(VolPan(vol, pan));
        return Error();
      }
      catch (const Error& err)
      {
        return err;
      }
    }
  private:
    typedef std::pair<LONG, LONG> VolPan;

    VolPan GetVolume() const
    {
      VolPan res;
      CheckWin32Error(Buffer->GetVolume(&res.first), THIS_LINE);
      CheckWin32Error(Buffer->GetPan(&res.second), THIS_LINE);
      return res;
    }

    void SetVolume(const VolPan& vols) const
    {
      CheckWin32Error(Buffer->SetVolume(vols.first), THIS_LINE);
      CheckWin32Error(Buffer->SetPan(vols.second), THIS_LINE);
    }
  private:
    const DirectSoundBufferPtr Buffer;
  };

  class DirectSoundBackendParameters
  {
  public:
    explicit DirectSoundBackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    String GetDevice() const
    {
      Parameters::StringType device;
      Accessor.FindValue(Parameters::ZXTune::Sound::Backends::DirectSound::DEVICE, device);
      return device;
    }

    uint_t GetLatency() const
    {
      Parameters::IntType latency = Parameters::ZXTune::Sound::Backends::DirectSound::LATENCY_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Sound::Backends::DirectSound::LATENCY, latency) &&
          !Math::InRange<Parameters::IntType>(latency, LATENCY_MIN, LATENCY_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("DirectSound backend error: latency (%1%) is out of range (%2%..%3%)."), static_cast<int_t>(latency), LATENCY_MIN, LATENCY_MAX);
      }
      return static_cast<uint_t>(latency);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class DirectSoundBackendWorker : public BackendWorker
  {
  public:
    DirectSoundBackendWorker(DirectSound::Api::Ptr api, Parameters::Accessor::Ptr params)
      : Api(api)
      , BackendParams(params)
      , RenderingParameters(RenderParameters::Create(BackendParams))
    {
    }

    virtual void Test()
    {
      OpenDevices();
    }

    virtual void OnStartup(const Module::Holder& /*module*/)
    {
      Dbg("Starting");
      Objects = OpenDevices();
      Dbg("Started");
    }

    virtual void OnShutdown()
    {
      Dbg("Stopping");
      Objects.Stream->Stop();
      Objects.Volume.reset();
      Objects.Stream.reset();
      Objects.Device.reset();
      Dbg("Stopped");
    }

    virtual void OnPause()
    {
      Objects.Stream->Pause();
    }

    virtual void OnResume()
    {
    }

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }

    virtual void OnBufferReady(Chunk& buffer)
    {
      if (SamplesShouldBeConverted)
      {
        std::transform(buffer.front().begin(), buffer.back().end(), buffer.front().begin(), &ToSignedSample);
      }
      Objects.Stream->Add(buffer);
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      return CreateVolumeControlDelegate(Objects.Volume);
    }
  private:
    struct DSObjects
    {
      DirectSoundDevicePtr Device;
      StreamBuffer::Ptr Stream;
      VolumeControl::Ptr Volume;
    };

    DSObjects OpenDevices()
    {
      const DirectSoundBackendParameters params(*BackendParams);
      const String device = params.GetDevice();
      DSObjects res;
      res.Device = OpenDevice(*Api, device);
      const uint_t latency = params.GetLatency();
      const DirectSoundBufferPtr buffer = CreateSecondaryBuffer(res.Device, RenderingParameters->SoundFreq(), latency);
      const uint_t frameDurationMs = RenderingParameters->FrameDurationMicrosec() / 1000;
      res.Stream = boost::make_shared<StreamBuffer>(buffer, boost::posix_time::millisec(frameDurationMs));
      const DirectSoundBufferPtr primary = CreatePrimaryBuffer(res.Device);
      res.Volume = boost::make_shared<DirectSoundVolumeControl>(primary);
      return res;
    }
  private:
    const DirectSound::Api::Ptr Api;
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    DSObjects Objects; 
  };

  const String ID = Text::DSOUND_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("DirectSound support backend.");

  class DirectSoundBackendCreator : public BackendCreator
  {
  public:
    explicit DirectSoundBackendCreator(DirectSound::Api::Ptr api)
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

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const BackendWorker::Ptr worker(new DirectSoundBackendWorker(Api, allParams));
        result = Sound::CreateBackend(params, worker);
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE,
          translate("Failed to create backend '%1%'."), Id()).AddSuberror(e);
      }
    }
  private:
    const DirectSound::Api::Ptr Api;
  };

  class DirectSoundDevice : public DirectSound::Device
  {
  public:
    DirectSoundDevice(const String& id, const String& name)
      : IdValue(id)
      , NameValue(name)
    {
    }

    virtual String Id() const
    {
      return IdValue;
    }

    virtual String Name() const
    {
      return NameValue;
    }
  private:
    const String IdValue;
    const String NameValue;
  };

  class DevicesIterator : public DirectSound::Device::Iterator
  {
  public:
    explicit DevicesIterator(DirectSound::Api::Ptr api)
      : Current(Devices.begin())
    {
      if (DS_OK != api->DirectSoundEnumerateA(&EnumerateDevicesCallback, &Devices))
      {
        Dbg("Failed to enumerate devices. Skip backend.");
        Current = Devices.end();
      }
      else
      {
        Dbg("Detected %1% devices to output.", Devices.size());
        Current = Devices.begin();
      }
    }

    virtual bool IsValid() const
    {
      return Current != Devices.end();
    }

    virtual DirectSound::Device::Ptr Get() const
    {
      return IsValid()
        ? boost::make_shared<DirectSoundDevice>(Current->first, Current->second)
        : DirectSound::Device::Ptr();
    }

    virtual void Next()
    {
      if (IsValid())
      {
        ++Current;
      }
    }
  private:
    static BOOL CALLBACK EnumerateDevicesCallback(LPGUID guid, LPCSTR descr, LPCSTR module, LPVOID param)
    {
      const String& id = Guid2String(guid);
      const String& name = FromStdString(descr);
      Dbg("Detected device '%1%' (uuid=%2% module='%3%')", name, id, module);
      DevicesArray& devices = *safe_ptr_cast<DevicesArray*>(param);
      devices.push_back(IdAndName(id, name));
      return TRUE;
    }
  private:
    typedef std::pair<String, String> IdAndName;
    typedef std::vector<IdAndName> DevicesArray;
    DevicesArray Devices;
    DevicesArray::const_iterator Current;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterDirectSoundBackend(BackendsEnumerator& enumerator)
    {
      try
      {
        const DirectSound::Api::Ptr api = DirectSound::LoadDynamicApi();
        if (DevicesIterator(api).IsValid())
        {
          const BackendCreator::Ptr creator(new DirectSoundBackendCreator(api));
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

    namespace DirectSound
    {
      Device::Iterator::Ptr EnumerateDevices()
      {
        try
        {
          const Api::Ptr api = LoadDynamicApi();
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
