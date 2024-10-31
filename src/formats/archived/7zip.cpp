/**
 *
 * @file
 *
 * @brief  7zip archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/container_base.h"
#include "binary/container_factories.h"
#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/view.h"
#include "debug/log.h"
#include "formats/archived.h"
#include "strings/encoding.h"
#include "strings/map.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"

#include "3rdparty/lzma/C/7z.h"
#include "3rdparty/lzma/C/7zCrc.h"
#include "3rdparty/lzma/C/7zTypes.h"

#include <stdlib.h>

#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

namespace Formats::Archived
{
  namespace SevenZip
  {
    const Debug::Stream Dbg("Formats::Archived::7zip");

    struct Header
    {
      uint8_t Signature[6];
      uint8_t MajorVersion;
      uint8_t MinorVersion;
      le_uint32_t StartHeaderCRC;
      le_uint64_t NextHeaderOffset;
      le_uint64_t NextHeaderSize;
      le_uint32_t NextHeaderCRC;
    };

    static_assert(sizeof(Header) * alignof(Header) == 0x20, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header);

    const auto DESCRIPTION = "7zip"sv;
    const auto FORMAT =
        "'7'z bc af 27 1c"  // signature
        "00 ?"              // version
        ""sv;

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

      static void* MyAlloc(ISzAllocPtr /*p*/, size_t size)
      {
        return size ? malloc(size) : nullptr;
      }

      static void MyFree(ISzAllocPtr /*p*/, void* address)
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
        : Data(std::move(data))
        , Start(static_cast<const uint8_t*>(Data->Start()))
        , Limit(Data->Size())
      {
        Read = DoRead;
        Seek = DoSeek;
      }

    private:
      static SeekStream& GetSelf(ISeekInStreamPtr p)
      {
        return *const_cast<SeekStream*>(static_cast<const SeekStream*>(p));
      }

      static SRes DoRead(ISeekInStreamPtr p, void* buf, size_t* size)
      {
        if (size_t originalSize = *size)
        {
          auto& self = GetSelf(p);
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

      static SRes DoSeek(ISeekInStreamPtr p, Int64* pos, ESzSeek origin)
      {
        auto& self = GetSelf(p);
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
      std::size_t Position = 0;
    };

    class LookupStream : public CLookToRead2
    {
    public:
      LookupStream(ISzAlloc& allocator, Binary::Data::Ptr data)
        : CLookToRead2()
        , Stream(std::move(data))
        , Allocator(allocator)
      {
        LookToRead2_CreateVTable(this, false);
        LookToRead2_INIT(this);
        realStream = &Stream;
        bufSize = 1 << 18;
        buf = static_cast<Byte*>(Allocator.Alloc(nullptr, bufSize));
      }

      ~LookupStream()
      {
        Allocator.Free(nullptr, buf);
      }

    private:
      SeekStream Stream;
      ISzAlloc& Allocator;
    };

    class Archive
    {
    public:
      using Ptr = std::shared_ptr<const Archive>;

      explicit Archive(Binary::Data::Ptr data)
        : Allocator(LzmaContext::Allocator())
        , Stream(*Allocator, std::move(data))
      {
        SzArEx_Init(&Db);
        CheckError(SzArEx_Open(&Db, &Stream.vt, Allocator, Allocator));
      }

      ~Archive()
      {
        Allocator->Free(nullptr, Cache.OutBuffer);
        SzArEx_Free(&Db, Allocator);
      }

      uint_t GetFilesCount() const
      {
        return Db.NumFiles;
      }

      String GetFileName(uint_t idx) const
      {
        const size_t nameLen = SzArEx_GetFileNameUtf16(&Db, idx, nullptr);
        Require(nameLen > 0);
        std::vector<UInt16> buf(nameLen);
        UInt16* const data = buf.data();
        SzArEx_GetFileNameUtf16(&Db, idx, data);
        return Strings::Utf16ToUtf8({data, nameLen - 1});
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
        // WARN: not thread-safe
        size_t offset = 0;
        size_t outSizeProcessed = 0;
        CheckError(SzArEx_Extract(&Db, const_cast<ILookInStreamPtr>(&Stream.vt), idx, &Cache.BlockIndex,
                                  &Cache.OutBuffer, &Cache.OutBufferSize, &offset, &outSizeProcessed, Allocator,
                                  Allocator));
        Require(outSizeProcessed == SzArEx_GetFileSize(&Db, idx));
        return Binary::CreateContainer(Binary::View(Cache.OutBuffer + offset, outSizeProcessed));
      }

    private:
      static void CheckError(SRes err)
      {
        // TODO: detailize
        Require(err == SZ_OK);
      }

      struct UnpackCache
      {
        UInt32 BlockIndex = ~UInt32(0);
        Byte* OutBuffer = nullptr;
        size_t OutBufferSize = 0;
      };

    private:
      ISzAlloc* const Allocator;
      LookupStream Stream;
      CSzArEx Db;
      mutable UnpackCache Cache;
    };

    class File : public Archived::File
    {
    public:
      File(Archive::Ptr archive, uint_t idx)
        : Arch(std::move(archive))
        , Idx(idx)
        , Name(Arch->GetFileName(Idx))
        , Size(Arch->GetFileSize(Idx))
      {
        Dbg("Created file '{}', idx={} size={}", Name, Idx, Size);
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
        Dbg("Decompressing '{}'", Name);
        try
        {
          return Arch->GetFileData(Idx);
        }
        catch (const std::exception&)
        {
          Dbg("Failed to decompress");
        }
        return {};
      }

    private:
      const Archive::Ptr Arch;
      const uint_t Idx;
      const String Name;
      const std::size_t Size;
    };

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      Container(Binary::Container::Ptr data, std::vector<File::Ptr> files)
        : BaseContainer(std::move(data))
        , Files(std::move(files))
      {
        for (const auto& file : Files)
        {
          Lookup.emplace(file->GetName(), file);
        }
      }

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (const auto& file : Files)
        {
          walker.OnFile(*file);
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        return Lookup.Get(name);
      }

      uint_t CountFiles() const override
      {
        return static_cast<uint_t>(Files.size());
      }

    private:
      std::vector<File::Ptr> Files;
      Strings::ValueMap<File::Ptr> Lookup;
    };
  }  // namespace SevenZip

  class SevenZipDecoder : public Decoder
  {
  public:
    SevenZipDecoder()
      : Format(Binary::CreateFormat(SevenZip::FORMAT, SevenZip::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return SevenZip::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      const Binary::View data(rawData);
      if (!Format->Match(data))
      {
        return {};
      }
      const auto& hdr = *data.As<SevenZip::Header>();
      const std::size_t totalSize = sizeof(hdr) + hdr.NextHeaderOffset + hdr.NextHeaderSize;
      auto archiveData = rawData.GetSubcontainer(0, totalSize);

      try
      {
        const SevenZip::Archive::Ptr archive = MakePtr<SevenZip::Archive>(archiveData);
        const auto totalFiles = archive->GetFilesCount();
        std::vector<File::Ptr> files;
        files.reserve(totalFiles);
        for (uint_t idx = 0; idx < totalFiles; ++idx)
        {
          if (archive->IsDir(idx) || 0 == archive->GetFileSize(idx))
          {
            continue;
          }
          files.emplace_back(MakePtr<SevenZip::File>(archive, idx));
        }
        return MakePtr<SevenZip::Container>(std::move(archiveData), std::move(files));
      }
      catch (const std::exception&)
      {
        return {};
      }
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr Create7zipDecoder()
  {
    return MakePtr<SevenZipDecoder>();
  }
}  // namespace Formats::Archived
