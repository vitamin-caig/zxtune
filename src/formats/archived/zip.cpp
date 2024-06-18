/**
 *
 * @file
 *
 * @brief  ZIP archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
// library includes
#include <binary/container_base.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/archived.h>
#include <formats/packed/decoders.h>
#include <formats/packed/zip_supp.h>
#include <strings/encoding.h>
#include <strings/map.h>
// std includes
#include <memory>
#include <numeric>

namespace Formats::Archived
{
  namespace Zip
  {
    const Debug::Stream Dbg("Formats::Archived::Zip");

    class File : public Archived::File
    {
    public:
      File(const Packed::Decoder& decoder, StringView name, std::size_t size, Binary::Container::Ptr data)
        : Decoder(decoder)
        , Name(name)
        , Size(size)
        , Data(std::move(data))
      {}

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return Size;
      }

      Binary::Container::Ptr GetData() const override
      {
        Dbg("Decompressing '{}'", Name);
        return Decoder.Decode(*Data);
      }

    private:
      const Formats::Packed::Decoder& Decoder;
      const String Name;
      const std::size_t Size;
      const Binary::Container::Ptr Data;
    };

    class BlocksIterator
    {
    public:
      explicit BlocksIterator(Binary::View data)
        : Stream(data)
      {}

      bool IsEof() const
      {
        if (!GetBlock<Packed::Zip::GenericHeader>())
        {
          return true;
        }
        const auto curBlockSize = GetBlockSize();
        return !curBlockSize || curBlockSize > Stream.GetRestSize();
      }

      template<class T>
      const T* GetBlock() const
      {
        const T* rawBlock = Stream.PeekField<T>();
        return rawBlock && rawBlock->Signature == T::SIGNATURE ? rawBlock : nullptr;
      }

      std::unique_ptr<const Packed::Zip::CompressedFile> GetFile() const
      {
        using namespace Packed::Zip;
        if (const auto* header = GetBlock<LocalFileHeader>())
        {
          return CompressedFile::Create(*header, Stream.GetRestSize());
        }
        return {};
      }

      std::size_t GetOffset() const
      {
        return Stream.GetPosition();
      }

      void Next()
      {
        Stream.Skip(GetBlockSize());
      }

    private:
      std::size_t GetBlockSize() const
      {
        using namespace Packed::Zip;
        if (const auto* header = GetBlock<LocalFileHeader>())
        {
          const auto file = CompressedFile::Create(*header, Stream.GetRestSize());
          return file ? file->GetPackedSize() : 0;
        }
        else if (const auto* footer = GetBlock<LocalFileFooter>())
        {
          return sizeof(*footer);
        }
        else if (const auto* extra = GetBlock<ExtraDataRecord>())
        {
          return extra->GetSize();
        }
        else if (const auto* centralHeader = GetBlock<CentralDirectoryFileHeader>())
        {
          return centralHeader->GetSize();
        }
        else if (const auto* centralFooter = GetBlock<CentralDirectoryEnd>())
        {
          return centralFooter->GetSize();
        }
        else if (const auto* signature = GetBlock<DigitalSignature>())
        {
          return signature->GetSize();
        }
        else
        {
          Dbg("Unknown block");
          return 0;
        }
      }

    private:
      Binary::DataInputStream Stream;
    };

    // TODO: make BlocksIterator
    class FileIterator
    {
    public:
      explicit FileIterator(const Packed::Decoder& decoder, const Binary::Container& data)
        : Decoder(decoder)
        , Data(data)
        , Blocks(data)
      {
        SkipNonFileHeaders();
      }

      bool IsEof() const
      {
        return Blocks.IsEof();
      }

      bool IsValid() const
      {
        assert(!IsEof());
        if (const auto* header = Blocks.GetBlock<Packed::Zip::LocalFileHeader>())
        {
          return header->IsSupported();
        }
        return false;
      }

      String GetName() const
      {
        assert(!IsEof());
        if (const auto* header = Blocks.GetBlock<Packed::Zip::LocalFileHeader>())
        {
          const StringView rawName(header->Name, header->NameSize);
          const bool isUtf8 = 0 != (header->Flags & Packed::Zip::FILE_UTF8);
          return isUtf8 ? String{rawName} : Strings::ToAutoUtf8(rawName);
        }
        assert(!"Failed to get name");
        return {};
      }

      File::Ptr GetFile() const
      {
        assert(IsValid());
        const auto file = Blocks.GetFile();
        if (file)
        {
          auto data = Data.GetSubcontainer(Blocks.GetOffset(), file->GetPackedSize());
          return MakePtr<File>(Decoder, GetName(), file->GetUnpackedSize(), std::move(data));
        }
        assert(!"Failed to get file");
        return {};
      }

      void Next()
      {
        assert(!IsEof());
        Blocks.Next();
        SkipNonFileHeaders();
      }

      std::size_t GetOffset() const
      {
        return Blocks.GetOffset();
      }

    private:
      void SkipNonFileHeaders()
      {
        while (!Blocks.IsEof() && !Blocks.GetBlock<Packed::Zip::LocalFileHeader>())
        {
          Blocks.Next();
        }
      }

    private:
      const Packed::Decoder& Decoder;
      const Binary::Container& Data;
      BlocksIterator Blocks;
    };

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      Container(Packed::Decoder::Ptr decoder, Binary::Container::Ptr data, uint_t filesCount)
        : BaseContainer(std::move(data))
        , Decoder(std::move(decoder))
        , FilesCount(filesCount)
      {
        Dbg("Found {} files. Size is {}", filesCount, Delegate->Size());
      }

      void ExploreFiles(const Container::Walker& walker) const override
      {
        FillCache();
        for (const auto& entry : Files)
        {
          walker.OnFile(*entry.second);
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        if (auto file = FindCachedFile(name))
        {
          return file;
        }
        return FindNonCachedFile(name);
      }

      uint_t CountFiles() const override
      {
        return FilesCount;
      }

    private:
      void FillCache() const
      {
        FindNonCachedFile({});
      }

      File::Ptr FindCachedFile(StringView name) const
      {
        if (Iter)
        {
          return Files.Get(name);
        }
        return {};
      }

      File::Ptr FindNonCachedFile(StringView name) const
      {
        CreateIterator();
        while (!Iter->IsEof())
        {
          const String fileName = Iter->GetName();
          if (!Iter->IsValid())
          {
            Dbg("Invalid file '{}'", fileName);
            Iter->Next();
            continue;
          }
          Dbg("Found file '{}'", fileName);
          auto fileObject = Iter->GetFile();
          Files.emplace(fileName, fileObject);
          Iter->Next();
          if (fileName == name)
          {
            return fileObject;
          }
        }
        return {};
      }

      void CreateIterator() const
      {
        if (!Iter)
        {
          Iter = std::make_unique<FileIterator>(*Decoder, *Delegate);
        }
      }

    private:
      const Formats::Packed::Decoder::Ptr Decoder;
      const uint_t FilesCount;
      mutable std::unique_ptr<FileIterator> Iter;
      mutable Strings::ValueMap<File::Ptr> Files;
    };
  }  // namespace Zip

  class ZipDecoder : public Decoder
  {
  public:
    ZipDecoder()
      : FileDecoder(Formats::Packed::CreateZipDecoder())
    {}

    String GetDescription() const override
    {
      return FileDecoder->GetDescription();
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return FileDecoder->GetFormat();
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      if (!FileDecoder->GetFormat()->Match(data))
      {
        return {};
      }

      uint_t filesCount = 0;
      Zip::BlocksIterator iter(data);
      for (; !iter.IsEof(); iter.Next())
      {
        if (const auto* file = iter.GetBlock<Packed::Zip::LocalFileHeader>())
        {
          filesCount += file->IsSupported();
        }
      }
      if (const std::size_t totalSize = iter.GetOffset())
      {
        const Binary::Container::Ptr archive = data.GetSubcontainer(0, totalSize);
        return MakePtr<Zip::Container>(FileDecoder, archive, filesCount);
      }
      else
      {
        return {};
      }
    }

  private:
    const Formats::Packed::Decoder::Ptr FileDecoder;
  };

  Decoder::Ptr CreateZipDecoder()
  {
    return MakePtr<ZipDecoder>();
  }
}  // namespace Formats::Archived
