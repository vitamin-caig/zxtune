/**
 *
 * @file
 *
 * @brief  WAV backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/file_backend.h"
#include "sound/backends/l10n.h"
#include "sound/backends/storage.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <binary/dump.h>
#include <l10n/markup.h>
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/render_params.h>
// std includes
#include <algorithm>
#include <cstring>

namespace Sound::Wav
{
  const auto BACKEND_ID = "wav"_id;
  const char* const DESCRIPTION = L10n::translate("WAV support backend");

  // Standard .wav header
  struct WaveFormat
  {
    uint8_t Id[4];            //'RIFF'
    le_uint32_t Size;         // file size - 8
    uint8_t Type[4];          //'WAVE'
    uint8_t ChunkId[4];       //'fmt '
    le_uint32_t ChunkSize;    // 16
    le_uint16_t Compression;  // 1
    le_uint16_t Channels;
    le_uint32_t Samplerate;
    le_uint32_t BytesPerSec;
    le_uint16_t Align;
    le_uint16_t BitsPerSample;
    uint8_t DataId[4];  //'data'
    le_uint32_t DataSize;
  };

  struct ListHeader
  {
    uint8_t Id[4];     // LIST
    le_uint32_t Size;  // next content size
    uint8_t Type[4];   // INFO
  };

  struct InfoElement
  {
    uint8_t Id[4];
    le_uint32_t Size;  // next content size - 1
    uint8_t Content[1];
  };

  const uint8_t RIFF[] = {'R', 'I', 'F', 'F'};
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
   - There may be additional subchunks in a Wave data stream. If so, each will have a char[4] SubChunkID, and unsigned
  long SubChunkSize, and SubChunkSize amount of data.
   - RIFF stands for Resource Interchange File Format.
  */

  class ListMetadata : public Binary::Dump
  {
  public:
    ListMetadata()
    {
      reserve(1024);
    }

    void SetTitle(const String& title)
    {
      AddElement(INAM, title);
    }

    void SetAuthor(const String& author)
    {
      AddElement(IART, author);
    }

    void SetComment(const String& comment)
    {
      AddElement(ICMT, comment);
    }

  private:
    // TODO: clarify about terminating zero
    void AddElement(const uint8_t* id, const String& str)
    {
      static_assert(sizeof(str[0]) == 1, "No multibyte strings allowed here");
      ListHeader* const hdr = GetHeader();
      const std::size_t strSize = str.size() + 1;
      InfoElement* const elem = AddElement(strSize);
      std::memcpy(elem->Id, id, sizeof(elem->Id));
      elem->Size = strSize;
      std::memcpy(elem->Content, str.c_str(), strSize);
      elem->Content[strSize] = 0;
      hdr->Size = size() - offsetof(ListHeader, Type);
    }

    ListHeader* GetHeader()
    {
      if (empty())
      {
        resize(sizeof(ListHeader));
        auto* const hdr = safe_ptr_cast<ListHeader*>(data());
        std::memcpy(hdr->Id, LIST, sizeof(LIST));
        std::memcpy(hdr->Type, INFO, sizeof(INFO));
        hdr->Size = 0;
        return hdr;
      }
      return safe_ptr_cast<ListHeader*>(data());
    }

    InfoElement* AddElement(std::size_t contentSize)
    {
      const std::size_t oldSize = size();
      const auto elemSize = Math::Align<std::size_t>(offsetof(InfoElement, Content) + contentSize, 2);
      resize(oldSize + elemSize);
      return safe_ptr_cast<InfoElement*>(&at(oldSize));
    }
  };

  class FileStream : public Sound::FileStream
  {
  public:
    FileStream(uint_t soundFreq, Binary::SeekableOutputStream::Ptr stream)
      : Stream(std::move(stream))
    {
      // init struct
      std::memcpy(Format.Id, RIFF, sizeof(RIFF));
      std::memcpy(Format.Type, WAVE, sizeof(WAVE));
      std::memcpy(Format.ChunkId, FORMAT, sizeof(FORMAT));
      Format.ChunkSize = 16;
      Format.Compression = 1;  // PCM
      Format.Channels = Sample::CHANNELS;
      std::memcpy(Format.DataId, DATA, sizeof(DATA));
      Format.Samplerate = soundFreq;
      Format.BytesPerSec = soundFreq * sizeof(Sample);
      Format.Align = sizeof(Sample);
      Format.BitsPerSample = Sample::BITS;
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

    void FlushMetadata() override {}

    void ApplyData(Chunk data) override
    {
      if (Sample::BITS == 16)
      {
        data.ToS16();
      }
      else
      {
        data.ToU8();
      }
      const Binary::View chunk(data);
      Stream->ApplyData(chunk);
      DoneBytes += static_cast<uint32_t>(chunk.Size());
    }

    void Flush() override
    {
      if (!Meta.empty())
      {
        Stream->ApplyData(Meta);
      }
      Stream->Flush();
      // write header
      Stream->Seek(0);
      Format.Size = sizeof(Format) - offsetof(WaveFormat, Type) + DoneBytes + Meta.size();
      Format.DataSize = DoneBytes;
      Stream->ApplyData(Format);
      Stream->Seek(DoneBytes + sizeof(Format));
    }

  private:
    const Binary::SeekableOutputStream::Ptr Stream;
    uint32_t DoneBytes = 0;
    WaveFormat Format;
    ListMetadata Meta;
  };

  class FileStreamFactory : public Sound::FileStreamFactory
  {
  public:
    explicit FileStreamFactory(uint_t frequency)
      : Frequency(frequency)
    {}

    BackendId GetId() const override
    {
      return BACKEND_ID;
    }

    FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const override
    {
      if (auto seekable = std::dynamic_pointer_cast<Binary::SeekableOutputStream>(stream))
      {
        return MakePtr<FileStream>(Frequency, std::move(seekable));
      }
      throw Error(THIS_LINE, translate("WAV conversion is not supported on non-seekable streams."));
    }

  private:
    const uint_t Frequency;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const override
    {
      auto factory = MakePtr<FileStreamFactory>(GetSoundFrequency(*params));
      return CreateFileBackendWorker(std::move(params), holder->GetModuleProperties(), std::move(factory));
    }
  };
}  // namespace Sound::Wav

namespace Sound
{
  void RegisterWavBackend(BackendsStorage& storage)
  {
    auto factory = MakePtr<Wav::BackendWorkerFactory>();
    storage.Register(Wav::BACKEND_ID, Wav::DESCRIPTION, CAP_TYPE_FILE, std::move(factory));
  }
}  // namespace Sound
