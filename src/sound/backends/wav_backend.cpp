/*
Abstract:
  Wave file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "file_backend.h"
#include "enumerator.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <sound/backend_attrs.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG EF5CB4C6

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::Wav");

  const Char WAV_BACKEND_ID[] = {'w', 'a', 'v', 0};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  // Standard .wav header
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
  const uint8_t RIFX[] = {'R', 'I', 'F', 'X'};
  const uint8_t WAVEfmt[] = {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '};
  const uint8_t DATA[] = {'d', 'a', 't', 'a'};

  /*
  From https://ccrma.stanford.edu/courses/422/projects/WaveFormat :

  Notes: 
   - The default byte ordering assumed for WAVE data files is little-endian.
     Files written using the big-endian byte ordering scheme have the identifier RIFX instead of RIFF. 
   - The sample data must end on an even byte boundary. Whatever that means. 
   - 8-bit samples are stored as unsigned bytes, ranging from 0 to 255.
     16-bit samples are stored as 2's-complement signed integers, ranging from -32768 to 32767. 
   - There may be additional subchunks in a Wave data stream. If so, each will have a char[4] SubChunkID, and unsigned long SubChunkSize, and SubChunkSize amount of data. 
   - RIFF stands for Resource Interchange File Format. 
  */
 
  const bool SamplesShouldBeConverted = sizeof(Sample) > 1 && !SAMPLE_SIGNED;

  class WavStream : public FileStream
  {
  public:
    WavStream(uint_t soundFreq, std::auto_ptr<std::ofstream> stream)
      : Stream(stream)
      , DoneBytes(0)
    {
      //init struct
      if (isLE())
      {
        std::memcpy(Format.Id, RIFF, sizeof(RIFF));
      }
      else
      {
        std::memcpy(Format.Id, RIFX, sizeof(RIFX));
      }
      std::memcpy(Format.Type, WAVEfmt, sizeof(WAVEfmt));
      Format.ChunkSize = fromLE<uint32_t>(16);
      Format.Compression = fromLE<uint16_t>(1);//PCM
      Format.Channels = fromLE<uint16_t>(OUTPUT_CHANNELS);
      std::memcpy(Format.DataId, DATA, sizeof(DATA));
      Format.Samplerate = fromLE(static_cast<uint32_t>(soundFreq));
      Format.BytesPerSec = fromLE(static_cast<uint32_t>(soundFreq * sizeof(MultiSample)));
      Format.Align = fromLE<uint16_t>(sizeof(MultiSample));
      Format.BitsPerSample = fromLE<uint16_t>(8 * sizeof(Sample));
      Flush();
    }

    virtual void SetTitle(const String& /*title*/)
    {
    }

    virtual void SetAuthor(const String& /*author*/)
    {
    }

    virtual void SetComment(const String& /*comment*/)
    {
    }

    virtual void FlushMetadata()
    {
    }

    virtual void ApplyData(const ChunkPtr& data)
    {
      if (SamplesShouldBeConverted)
      {
        std::transform(data->front().begin(), data->back().end(), data->front().begin(), &ToSignedSample);
      }
      const std::size_t sizeInBytes = data->size() * sizeof(data->front());
      Stream->write(safe_ptr_cast<const char*>(&data->front()), static_cast<std::streamsize>(sizeInBytes));
      DoneBytes += static_cast<uint32_t>(sizeInBytes);
    }

    virtual void Flush()
    {
      const std::streampos oldPos = Stream->tellp();
      // write header
      Stream->seekp(0);
      Format.Size = fromLE<uint32_t>(sizeof(Format) - 8 + DoneBytes);
      Format.DataSize = fromLE<uint32_t>(DoneBytes);
      Stream->write(safe_ptr_cast<const char*>(&Format), sizeof(Format));
      Stream->seekp(oldPos);
    }
  private:
    const std::auto_ptr<std::ofstream> Stream;
    uint32_t DoneBytes;
    WaveFormat Format;
  };

  class WavFileFactory : public FileStreamFactory
  {
  public:
    explicit WavFileFactory(Parameters::Accessor::Ptr params)
      : RenderingParameters(RenderParameters::Create(params))
    {
    }

    virtual String GetId() const
    {
      return WAV_BACKEND_ID;
    }

    virtual FileStream::Ptr OpenStream(const String& fileName, bool overWrite) const
    {
      std::auto_ptr<std::ofstream> rawFile = IO::CreateFile(fileName, overWrite);
      return FileStream::Ptr(new WavStream(RenderingParameters->SoundFreq(), rawFile));
    }
  private:
    const RenderParameters::Ptr RenderingParameters;
  };

  class WAVBackendCreator : public BackendCreator
  {
  public:
    virtual String Id() const
    {
      return WAV_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::WAV_BACKEND_DESCRIPTION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_FILE;
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const FileStreamFactory::Ptr factory = boost::make_shared<WavFileFactory>(allParams);
        const BackendWorker::Ptr worker = CreateFileBackendWorker(allParams, factory);
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
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterWAVBackend(BackendsEnumerator& enumerator)
    {
      const BackendCreator::Ptr creator(new WAVBackendCreator());
      enumerator.RegisterCreator(creator);
    }
  }
}
