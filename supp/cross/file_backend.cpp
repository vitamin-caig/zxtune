#include "../sound_backend_impl.h"

#include <tools.h>

#include <cassert>
#include <fstream>

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

  class FileBackend : public BackendImpl
  {
  public:
    FileBackend()
    {
      std::memcpy(Format.Id, RIFF, sizeof(RIFF));
      Format.Size = sizeof(Format) - 8;
      std::memcpy(Format.Type, WAVEfmt, sizeof(WAVEfmt));
      Format.ChunkSize = 16;
      Format.Compression = 1;//PCM
      Format.Channels = OUTPUT_CHANNELS;
      std::memcpy(Format.DataId, DATA, sizeof(DATA));
    }

    virtual void OnParametersChanged(unsigned changedFields)
    {
      OnShutdown();
      assert(!Params.DriverParameters.empty());
      File.open(Params.DriverParameters.c_str(), std::ios::binary);
      assert(File.is_open());
      File.seekp(sizeof(Format));

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
        File.seekp(0);
        File.write(safe_ptr_cast<const char*>(&Format), sizeof(Format));
        //File.close();
      }
    }

    virtual void OnBufferReady(const void* data, std::size_t sizeInBytes)
    {
      File.write(static_cast<const char*>(data), static_cast<std::streamsize>(sizeInBytes));
      Format.Size += static_cast<uint32_t>(sizeInBytes);
      Format.DataSize += static_cast<uint32_t>(sizeInBytes);
    }
  private:
    std::ofstream File;
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
