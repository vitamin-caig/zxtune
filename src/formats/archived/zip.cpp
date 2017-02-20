/**
* 
* @file
*
* @brief  ZIP archives support
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <binary/typed_container.h>
#include <debug/log.h>
#include <formats/archived.h>
#include <formats/packed/decoders.h>
#include <formats/packed/zip_supp.h>
#include <strings/encoding.h>
//std includes
#include <map>
#include <numeric>

namespace Formats
{
namespace Archived
{
  namespace Zip
  {
    const Debug::Stream Dbg("Formats::Archived::Zip");

    class File : public Archived::File
    {
    public:
      File(const Packed::Decoder& decoder, String name, std::size_t size, Binary::Container::Ptr data)
        : Decoder(decoder)
        , Name(std::move(name))
        , Size(size)
        , Data(std::move(data))
      {
      }

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
        Dbg("Decompressing '%1%'", Name);
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
      explicit BlocksIterator(const Binary::Container& data)
        : Container(data)
        , Limit(data.Size())
        , Offset(0)
      {
      }

      bool IsEof() const
      {
        if (!GetBlock<Packed::Zip::GenericHeader>())
        {
          return true;
        }
        if (const std::size_t size = GetBlockSize())
        {
          return Offset + size > Limit;
        }
        return true;
      }

      template<class T>
      const T* GetBlock() const
      {
        if (const T* rawBlock = Container.GetField<T>(Offset))
        {
          return fromLE(rawBlock->Signature) == T::SIGNATURE
            ? rawBlock
            : nullptr;
        }
        return nullptr;
      }

      std::unique_ptr<const Packed::Zip::CompressedFile> GetFile() const
      {
        using namespace Packed::Zip;
        if (const LocalFileHeader* header = GetBlock<LocalFileHeader>())
        {
          return CompressedFile::Create(*header, Limit - Offset);
        }
        return std::unique_ptr<const CompressedFile>();
      }

      std::size_t GetOffset() const
      {
        return Offset;
      }

      void Next()
      {
        const std::size_t size = GetBlockSize();
        Offset += size;
      }
    private:
      std::size_t GetBlockSize() const
      {
        using namespace Packed::Zip;
        if (const LocalFileHeader* header = GetBlock<LocalFileHeader>())
        {
          const std::unique_ptr<const CompressedFile> file = CompressedFile::Create(*header, Limit - Offset);
          return file.get()
            ? file->GetPackedSize()
            : 0;
        }
        else if (const LocalFileFooter* footer = GetBlock<LocalFileFooter>())
        {
          return sizeof(*footer);
        }
        else if (const ExtraDataRecord* extra = GetBlock<ExtraDataRecord>())
        {
          return extra->GetSize();
        }
        else if (const CentralDirectoryFileHeader* centralHeader = GetBlock<CentralDirectoryFileHeader>())
        {
          return centralHeader->GetSize();
        }
        else if (const CentralDirectoryEnd* centralFooter = GetBlock<CentralDirectoryEnd>())
        {
          return centralFooter->GetSize();
        }
        else if (const DigitalSignature* signature = GetBlock<DigitalSignature>())
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
      const Binary::TypedContainer Container;
      const std::size_t Limit;
      std::size_t Offset;
    };

    //TODO: make BlocksIterator
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
        if (const Packed::Zip::LocalFileHeader* header = Blocks.GetBlock<Packed::Zip::LocalFileHeader>())
        {
          return header->IsSupported();
        }
        return false;
      }

      String GetName() const
      {
        assert(!IsEof());
        if (const Packed::Zip::LocalFileHeader* header = Blocks.GetBlock<Packed::Zip::LocalFileHeader>())
        {
          const StringView rawName(header->Name, fromLE(header->NameSize));
          const bool isUtf8 = 0 != (fromLE(header->Flags) & Packed::Zip::FILE_UTF8);
          return isUtf8
            ? rawName.to_string()
            : Strings::ToAutoUtf8(rawName);
        }
        assert(!"Failed to get name");
        return String();
      }

      File::Ptr GetFile() const
      {
        assert(IsValid());
        const std::unique_ptr<const Packed::Zip::CompressedFile> file = Blocks.GetFile();
        if (file.get())
        {
          const Binary::Container::Ptr data = Data.GetSubcontainer(Blocks.GetOffset(), file->GetPackedSize());
          return MakePtr<File>(Decoder, GetName(), file->GetUnpackedSize(), data);
        }
        assert(!"Failed to get file");
        return File::Ptr();
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

    class Container : public Archived::Container
    {
    public:
      Container(Packed::Decoder::Ptr decoder, Binary::Container::Ptr data, uint_t filesCount)
        : Decoder(std::move(decoder))
        , Delegate(std::move(data))
        , FilesCount(filesCount)
      {
        Dbg("Found %1% files. Size is %2%", filesCount, Delegate->Size());
      }

      //Binary::Container
      const void* Start() const override
      {
        return Delegate->Start();
      }

      std::size_t Size() const override
      {
        return Delegate->Size();
      }

      Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
      {
        return Delegate->GetSubcontainer(offset, size);
      }

      //Archive::Container
      void ExploreFiles(const Container::Walker& walker) const override
      {
        FillCache();
        for (FilesMap::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
        {
          walker.OnFile(*it->second);
        }
      }

      File::Ptr FindFile(const String& name) const override
      {
        if (const File::Ptr file = FindCachedFile(name))
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
        FindNonCachedFile(String());
      }

      File::Ptr FindCachedFile(const String& name) const
      {
        if (Iter.get())
        {
          const FilesMap::const_iterator it = Files.find(name);
          if (it != Files.end())
          {
            return it->second;
          }
        }
        return File::Ptr();
      }

      File::Ptr FindNonCachedFile(const String& name) const
      {
        CreateIterator();
        while (!Iter->IsEof())
        {
          const String fileName = Iter->GetName();
          if (!Iter->IsValid())
          {
            Dbg("Invalid file '%1%'", fileName);
            Iter->Next();
            continue;
          }
          Dbg("Found file '%1%'", fileName);
          const File::Ptr fileObject = Iter->GetFile();
          Files.insert(FilesMap::value_type(fileName, fileObject));
          Iter->Next();
          if (fileName == name)
          {
            return fileObject;
          }
        }
        return File::Ptr();
      }

      void CreateIterator() const
      {
        if (!Iter.get())
        {
          Iter.reset(new FileIterator(*Decoder, *Delegate));
        }
      }
    private:
      const Formats::Packed::Decoder::Ptr Decoder;
      const Binary::Container::Ptr Delegate;
      const uint_t FilesCount;
      mutable std::unique_ptr<FileIterator> Iter;
      typedef std::map<String, File::Ptr> FilesMap;
      mutable FilesMap Files;
    };
  }//namespace Zip

  class ZipDecoder : public Decoder
  {
  public:
    ZipDecoder()
      : FileDecoder(Formats::Packed::CreateZipDecoder())
    {
    }

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
        return Container::Ptr();
      }

      uint_t filesCount = 0;
      Zip::BlocksIterator iter(data);
      for (; !iter.IsEof(); iter.Next())
      {
        if (const Packed::Zip::LocalFileHeader* file = iter.GetBlock<Packed::Zip::LocalFileHeader>())
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
        return Container::Ptr();
      }
    }
  private:
    const Formats::Packed::Decoder::Ptr FileDecoder;
  };

  Decoder::Ptr CreateZipDecoder()
  {
    return MakePtr<ZipDecoder>();
  }
}//namespace Archived
}//namespace Formats
