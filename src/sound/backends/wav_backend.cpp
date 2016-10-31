/**
*
* @file
*
* @brief  WAV backend implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "file_backend.h"
#include "storage.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <binary/data_adapter.h>
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
#include <cstring>
//text includes
#include "text/backends.h"

#define FILE_TAG EF5CB4C6

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace Wav
{
  const String ID = Text::WAV_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("WAV support backend");

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

  PACK_PRE struct ListHeader
  {
    uint8_t Id[4];   //LIST
    uint32_t Size;   //next content size
    uint8_t Type[4]; //INFO
  } PACK_POST;

  PACK_PRE struct InfoElement
  {
    uint8_t Id[4];
    uint32_t Size; //next content size - 1
    uint8_t Content[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t RIFF[] = {'R', 'I', 'F', 'F'};
  const uint8_t RIFX[] = {'R', 'I', 'F', 'X'};
  const uint8_t WAVE[] = {'W', 'A', 'V', 'E'};
  const uint8_t FORMAT[] = {'f', 'm', 't', ' '};
  const uint8_t DATA[] = {'d', 'a', 't', 'a'};

  const uint8_t LIST[] = {'L', 'I', 'S', 'T'};
  const uint8_t INFO[] = {'I', 'N', 'F', 'O'};
  const uint8_t INAM[] = {'I', 'N', 'A', 'M'};
  const uint8_t IART[] = {'I', 'A', 'R', 'T'};
  const uint8_t ICMT[] = {'I', 'C', 'M', 'T'};

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
 
  class ListMetadata : public Dump
  {
  public:
    ListMetadata()
    {
      reserve(1024);
    }

    void SetTitle(const String& title)
    {
      AddElement(INAM, ToStdString(title));
    }

    void SetAuthor(const String& author)
    {
      AddElement(IART, ToStdString(author));
    }

    void SetComment(const String& comment)
    {
      AddElement(ICMT, ToStdString(comment));
    }
  private:
    void AddElement(const uint8_t* id, const std::string& str)
    {
      ListHeader* const hdr = GetHeader();
      const std::size_t strSize = str.size() + 1;
      InfoElement* const elem = AddElement(strSize);
      std::memcpy(elem->Id, id, sizeof(elem->Id));
      elem->Size = fromLE(strSize);
      std::memcpy(elem->Content, str.c_str(), strSize);
      elem->Content[strSize] = 0;
      hdr->Size = fromLE(size() - offsetof(ListHeader, Type));
    }

    ListHeader* GetHeader()
    {
      if (empty())
      {
        resize(sizeof(ListHeader));
        ListHeader* const hdr = safe_ptr_cast<ListHeader*>(&front());
        std::memcpy(hdr->Id, LIST, sizeof(LIST));
        std::memcpy(hdr->Type, INFO, sizeof(INFO));
        hdr->Size = 0;
        return hdr;
      }
      return safe_ptr_cast<ListHeader*>(&front());
    }

    InfoElement* AddElement(std::size_t contentSize)
    {
      const std::size_t oldSize = size();
      const std::size_t elemSize = Math::Align<std::size_t>(offsetof(InfoElement, Content) + contentSize, 2);
      resize(oldSize + elemSize);
      return safe_ptr_cast<InfoElement*>(&at(oldSize));
    }
  };

  class FileStream : public Sound::FileStream
  {
  public:
    FileStream(uint_t soundFreq, Binary::SeekableOutputStream::Ptr stream)
      : Stream(std::move(stream))
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
      std::memcpy(Format.Type, WAVE, sizeof(WAVE));
      std::memcpy(Format.ChunkId, FORMAT, sizeof(FORMAT));
      Format.ChunkSize = fromLE<uint32_t>(16);
      Format.Compression = fromLE<uint16_t>(1);//PCM
      Format.Channels = fromLE<uint16_t>(Sample::CHANNELS);
      std::memcpy(Format.DataId, DATA, sizeof(DATA));
      Format.Samplerate = fromLE(static_cast<uint32_t>(soundFreq));
      Format.BytesPerSec = fromLE(static_cast<uint32_t>(soundFreq * sizeof(Sample)));
      Format.Align = fromLE<uint16_t>(sizeof(Sample));
      Format.BitsPerSample = fromLE<uint16_t>(Sample::BITS);
      Flush();
    }

    void SetTitle(const String& title) override
    {
      Meta.SetTitle(title);
    }

    void SetAuthor(const String& author) override
    {
      Meta.SetAuthor(author);
    }

    void SetComment(const String& comment) override
    {
      Meta.SetComment(comment);
    }

    void FlushMetadata() override
    {
    }

    void ApplyData(Chunk::Ptr data) override
    {
      if (Sample::BITS == 16)
      {
        data->ToS16();
      }
      else
      {
        data->ToU8();
      }
      const std::size_t sizeInBytes = data->size() * sizeof(data->front());
      Stream->ApplyData(Binary::DataAdapter(&data->front(), sizeInBytes));
      DoneBytes += static_cast<uint32_t>(sizeInBytes);
    }

    void Flush() override
    {
      if (!Meta.empty())
      {
        Stream->ApplyData(Binary::DataAdapter(&Meta.front(), Meta.size()));
      }
      Stream->Flush();
      // write header
      Stream->Seek(0);
      Format.Size = fromLE<uint32_t>(sizeof(Format) - offsetof(WaveFormat, Type) + DoneBytes + Meta.size());
      Format.DataSize = fromLE<uint32_t>(DoneBytes);
      Stream->ApplyData(Binary::DataAdapter(&Format, sizeof(Format)));
      Stream->Seek(DoneBytes + sizeof(Format));
    }
  private:
    const Binary::SeekableOutputStream::Ptr Stream;
    uint32_t DoneBytes;
    WaveFormat Format;
    ListMetadata Meta;
  };

  class FileStreamFactory : public Sound::FileStreamFactory
  {
  public:
    explicit FileStreamFactory(Parameters::Accessor::Ptr params)
      : RenderingParameters(RenderParameters::Create(params))
    {
    }

    String GetId() const override
    {
      return ID;
    }

    FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const override
    {
      if (const Binary::SeekableOutputStream::Ptr seekable = std::dynamic_pointer_cast<Binary::SeekableOutputStream>(stream))
      {
        return MakePtr<FileStream>(RenderingParameters->SoundFreq(), seekable);
      }
      throw Error(THIS_LINE, translate("WAV conversion is not supported on non-seekable streams."));
    }
  private:
    const RenderParameters::Ptr RenderingParameters;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr /*holder*/) const override
    {
      const FileStreamFactory::Ptr factory = MakePtr<FileStreamFactory>(params);
      return CreateFileBackendWorker(params, factory);
    }
  };
}//Wav
}//Sound

namespace Sound
{
  void RegisterWavBackend(BackendsStorage& storage)
  {
    const BackendWorkerFactory::Ptr factory = MakePtr<Wav::BackendWorkerFactory>();
    storage.Register(Wav::ID, Wav::DESCRIPTION, CAP_TYPE_FILE, factory);
  }
}
