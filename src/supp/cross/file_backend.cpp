#include "../sound_backend_impl.h"
#include "../sound_backend_types.h"

#include <tools.h>

#include <sound_attrs.h>

#include <boost/noncopyable.hpp>

#include <cassert>
#include <fstream>
#include <iostream>

namespace
{
  using namespace ZXTune::Sound;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct WaveFormat
  {
    uint8_t Id[4];          //'RIFF'
    uint32_t Size;          //file size - 8
    uint8_t Type[4];        //'WAVE'
    uint8_t ChunkId[4];     //'fmt '
    uint32_t ChunkSize;     //16
    uint16_t Compression;   //1
    uint16_t Channels;
    uint32_t Samplerate;
    uint32_t BytesPerSec;
    uint16_t Align;
    uint16_t BitsPerSample;
    uint8_t DataId[4];      //'data'
    uint32_t DataSize;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t RIFF[] = {'R', 'I', 'F', 'F'};
  const uint8_t WAVEfmt[] = {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '};
  const uint8_t DATA[] = {'d', 'a', 't', 'a'};

  class FileBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    FileBackend() : Stream(0), File(), RawOutput(false)
    {
      std::memcpy(Format.Id, RIFF, sizeof(RIFF));
      Format.Size = sizeof(Format) - 8;
      std::memcpy(Format.Type, WAVEfmt, sizeof(WAVEfmt));
      Format.ChunkSize = 16;
      Format.Compression = 1;//PCM
      Format.Channels = OUTPUT_CHANNELS;
      std::memcpy(Format.DataId, DATA, sizeof(DATA));
    }

    virtual ~FileBackend()
    {
      try
      {
        Stop();
      }
      catch (...)
      {
        //TODO
      }
    }

    virtual void OnParametersChanged(unsigned /*changedFields*/)
    {
      //loop is disabled
      Params.SoundParameters.Flags &= ~MOD_LOOP;
      if (Stream)
      {
        OnShutdown();
        //force raw mode if stdout
        RawOutput = Params.DriverParameters.empty() || (Params.DriverFlags & RAW_STREAM) ;//TODO
        OnStartup();
      }
    }

    virtual void OnStartup()
    {
      assert(!Stream);
      if (Params.DriverParameters.empty())
      {
        //not unicode or smth version
        Stream = &std::cout;
      }
      else
      {
        File.open(Params.DriverParameters.c_str(), std::ios::binary);
        assert(File.is_open());
        Stream = &File;
      }
      if (!RawOutput)
      {
        assert(File.is_open());
        File.seekp(sizeof(Format));
      }

      Format.Samplerate = Params.SoundParameters.SoundFreq;
      Format.BytesPerSec = Format.Samplerate * sizeof(SampleArray);
      Format.Align = sizeof(SampleArray);
      Format.BitsPerSample = 8 * sizeof(Sample);
      Format.DataSize = 0;
    }

    virtual void OnShutdown()
    {
      if (File.is_open())
      {
        if (!RawOutput)
        {
          File.seekp(0);
          File.write(safe_ptr_cast<const char*>(&Format), sizeof(Format));
        }
        File.close();
      }
      Stream = 0;
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnBufferReady(const void* data, std::size_t sizeInBytes)
    {
      assert(Stream);
#ifdef BOOST_BIG_ENDIAN
      if (sizeof(Sample) > 1)
      {
        assert(0 == sizeInBytes % sizeof(Sample));
        std::vector<Sample> tmp(sizeInBytes / sizeof(Sample));
        const Sample* const sampleData(static_cast<const Sample*>(data));
        std::transform(sampleData, sampleData + sizeInBytes / sizeof(Sample), tmp.begin(), &bytesSwap<Sample>);
        Stream->write(safe_ptr_cast<const char*>(&tmp[0]), static_cast<std::streamsize>(sizeInBytes));
      }
      else
      {
        Stream->write(static_cast<const char*>(data), static_cast<std::streamsize>(sizeInBytes));
      }
#else
      Stream->write(static_cast<const char*>(data), static_cast<std::streamsize>(sizeInBytes));
#endif
      Format.Size += static_cast<uint32_t>(sizeInBytes);
      Format.DataSize += static_cast<uint32_t>(sizeInBytes);
    }
  private:
    std::ostream* Stream;
    std::ofstream File;
    bool RawOutput;
    WaveFormat Format;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Backend::Ptr CreateFileBackend()
    {
      return Backend::Ptr(new FileBackend);
    }
  }
}
