/**
* 
* @file
*
* @brief  7zip archives support
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <debug/log.h>
#include <formats/archived.h>
//3rdparty includes
#include <3rdparty/lzma/C/7z.h>
#include <3rdparty/lzma/C/7zCrc.h>
//std includes
#include <cstring>
#include <list>
#include <map>
#include <numeric>
//text include
#include <formats/text/archived.h>

namespace Formats
{
namespace Archived
{
  namespace SevenZip
  {
    const Debug::Stream Dbg("Formats::Archived::7zip");

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Header
    {
      uint8_t Signature[6];
      uint8_t MajorVersion;
      uint8_t MinorVersion;
      uint32_t StartHeaderCRC;
      uint64_t NextHeaderOffset;
      uint64_t NextHeaderSize;
      uint32_t NextHeaderCRC;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(Header) == 0x20, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header);

    const std::string FORMAT(
        "'7'z bc af 27 1c" //signature
        "00 ?" //version
    );

    class LzmaContext : private ISzAlloc
    {
    public:
      static ISzAlloc* Allocator()
      {
        static LzmaContext Instance;
        return &Instance;
      }
    private:
      LzmaContext()
      {
        Alloc = MyAlloc;
        Free = MyFree;
        CrcGenerateTable();
      }
      static void* MyAlloc(void* /*p*/, size_t size)
      {
        return size ? malloc(size) : 0;
      }

      static void MyFree(void* /*p*/, void* address)
      {
        if (address)
        {
          free(address);
        }
      }
    };

    class SeekStream : public ISeekInStream
    {
    public:
      explicit SeekStream(Binary::Data::Ptr data)
        : Data(data)
        , Start(static_cast<const uint8_t*>(Data->Start()))
        , Limit(Data->Size())
        , Position()
      {
        Read = DoRead;
        Seek = DoSeek;
      }

    private:
      static SRes DoRead(void *p, void *buf, size_t *size) {
        if (size_t originalSize = *size)
        {
          SeekStream& self = *static_cast<SeekStream*>(p);
          if (const std::size_t avail = self.Limit - self.Position)
          {
            originalSize = std::min<size_t>(originalSize, avail);
            std::memcpy(buf, self.Start + self.Position, originalSize);
            self.Position += originalSize;
            *size = originalSize;
          }
          else
          {
            return SZ_ERROR_INPUT_EOF;
          }
        }
        return SZ_OK;
      }

      static SRes DoSeek(void *p, Int64 *pos, ESzSeek origin) {
        SeekStream& self = *static_cast<SeekStream*>(p);
        Int64 newPos = *pos;
        switch (origin)
        {
        case SZ_SEEK_SET:
          break;
        case SZ_SEEK_CUR:
          newPos += self.Position;
          break;
        case SZ_SEEK_END:
          newPos += self.Limit;
          break;
        }
        *pos = self.Position = static_cast<std::size_t>(std::min<Int64>(newPos, self.Limit));
        return SZ_OK;
      }
    private:
      const Binary::Data::Ptr Data;
      const uint8_t* const Start;
      const std::size_t Limit;
      std::size_t Position;
    };

    class LookupStream : public CLookToRead
    {
    public:
      explicit LookupStream(Binary::Data::Ptr data)
        : Stream(data)
      {
        LookToRead_CreateVTable(this, false);
        realStream = &Stream;
        LookToRead_Init(this);
      }
    private:
      SeekStream Stream;
    };

    class Archive
    {
    public:
      typedef std::shared_ptr<const Archive> Ptr;

      explicit Archive(Binary::Data::Ptr data)
        : Stream(data)
      {
        SzArEx_Init(&Db);
        CheckError(SzArEx_Open(&Db, &Stream.s, LzmaContext::Allocator(), LzmaContext::Allocator()));
      }

      ~Archive()
      {
        LzmaContext::Allocator()->Free(0, Cache.OutBuffer);
        SzArEx_Free(&Db, LzmaContext::Allocator());
      }

      uint_t GetFilesCount() const
      {
        return Db.NumFiles;
      }

      String GetFileName(uint_t idx) const
      {
        const size_t nameLen = SzArEx_GetFileNameUtf16(&Db, idx, 0);
        Require(nameLen > 0);
        std::vector<UInt16> buf(nameLen);
        UInt16* const data = &buf[0];
        SzArEx_GetFileNameUtf16(&Db, idx, data);
        //TODO:
        return String(data, data + nameLen - 1);
      }

      bool IsDir(uint_t idx) const
      {
        return SzArEx_IsDir(&Db, idx);
      }

      std::size_t GetFileSize(uint_t idx) const
      {
        return SzArEx_GetFileSize(&Db, idx);
      }

      Binary::Container::Ptr GetFileData(uint_t idx) const
      {
        //WARN: not thread-safe
        size_t offset = 0;
        size_t outSizeProcessed = 0;
        CheckError(SzArEx_Extract(&Db, const_cast<ILookInStream*>(&Stream.s), idx, &Cache.BlockIndex, &Cache.OutBuffer, &Cache.OutBufferSize, &offset, &outSizeProcessed, LzmaContext::Allocator(), LzmaContext::Allocator()));
        Require(outSizeProcessed == SzArEx_GetFileSize(&Db, idx));
        return Binary::CreateContainer(Cache.OutBuffer + offset, outSizeProcessed);
      }
    private:
      static void CheckError(SRes err)
      {
        //TODO: detailize
        Require(err == SZ_OK);
      }

      struct UnpackCache
      {
        UInt32 BlockIndex;
        Byte* OutBuffer;
        size_t OutBufferSize;

        UnpackCache()
          : BlockIndex(~UInt32(0))
          , OutBuffer(0)
          , OutBufferSize(0)
        {
        }
      };
    private:
      LookupStream Stream;
      CSzArEx Db;
      mutable UnpackCache Cache;
    };

    class File : public Archived::File
    {
    public:
      File(Archive::Ptr archive, uint_t idx)
        : Arch(archive)
        , Idx(idx)
        , Name(Arch->GetFileName(Idx))
        , Size(Arch->GetFileSize(Idx))
      {
        Dbg("Created file '%1%', idx=%2% size=%3%", Name, Idx, Size);
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
        return Arch->GetFileData(Idx);
      }
    private:
      const Archive::Ptr Arch;
      const uint_t Idx;
      const String Name;
      const std::size_t Size;
    };

    class Container : public Archived::Container
    {
    public:
      template<class It>
      Container(Binary::Container::Ptr data, It begin, It end)
        : Delegate(data)
      {
        for (It it = begin; it != end; ++it)
        {
          const File::Ptr file = *it;
          Files.insert(FilesMap::value_type(file->GetName(), file));
        }
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
        for (FilesMap::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
        {
          walker.OnFile(*it->second);
        }
      }

      File::Ptr FindFile(const String& name) const override
      {
        const FilesMap::const_iterator it = Files.find(name);
        return it != Files.end()
          ? it->second
          : File::Ptr();
      }

      uint_t CountFiles() const override
      {
        return static_cast<uint_t>(Files.size());
      }
    private:
      const Binary::Container::Ptr Delegate;
      typedef std::map<String, File::Ptr> FilesMap;
      FilesMap Files;
    };
  }//namespace SevenZip

  class SevenZipDecoder : public Decoder
  {
  public:
    SevenZipDecoder()
      : Format(Binary::CreateFormat(SevenZip::FORMAT, SevenZip::MIN_SIZE))
    {
    }

    String GetDescription() const override
    {
      return Text::SEVENZIP_DECODER_DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      if (!Format->Match(data))
      {
        return Container::Ptr();
      }
      const SevenZip::Header& hdr = *static_cast<const SevenZip::Header*>(data.Start());
      const std::size_t totalSize = sizeof(hdr) + fromLE(hdr.NextHeaderOffset) + fromLE(hdr.NextHeaderSize);
      const Binary::Container::Ptr archiveData = data.GetSubcontainer(0, totalSize);

      std::list<File::Ptr> files;
      const SevenZip::Archive::Ptr archive = MakePtr<SevenZip::Archive>(archiveData);
      for (uint_t idx = 0, lim = archive->GetFilesCount(); idx < lim; ++idx)
      {
        if (archive->IsDir(idx) || 0 == archive->GetFileSize(idx))
        {
          continue;
        }
        const File::Ptr file = MakePtr<SevenZip::File>(archive, idx);
        files.push_back(file);
      }
      return MakePtr<SevenZip::Container>(archiveData, files.begin(), files.end());
    }
  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr Create7zipDecoder()
  {
    return MakePtr<SevenZipDecoder>();
  }
}//namespace Archived
}//namespace Formats
