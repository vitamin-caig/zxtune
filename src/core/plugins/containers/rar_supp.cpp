/*
Abstract:
  Rar convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "core/plugins/registrator.h"
#include "core/src/path.h"
//common includes
#include <byteorder.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <formats/packed_decoders.h>
//std includes
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

//platform includes
#include <zlib.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'R', 'A', 'R', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::RAR_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK;

  const std::string THIS_MODULE("Core::RARSupp");

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct BlockHeader
  {
    uint16_t CRC;
    uint8_t Type;
    uint16_t Flags;
    uint16_t Size;

    enum
    {
      FLAG_HAS_TAIL = 0x8000
    };

    bool IsExtended() const
    {
      return 0 != (fromLE(Flags) & FLAG_HAS_TAIL);
    }
  } PACK_POST;

  const BlockHeader MARKER = 
  {
    fromLE(0x6152),
    0x72,
    fromLE(0x1a21),
    fromLE(0x0007)
  };

  PACK_PRE struct ExtendedBlockHeader : BlockHeader
  {
    uint32_t AdditionalSize;
  } PACK_POST;

  // ArchiveBlockHeader is always BlockHeader
  PACK_PRE struct ArchiveBlockHeader : BlockHeader
  {
    static const uint8_t TYPE = 0x73;

    //CRC from Type till Reserved2
    uint16_t Reserved1;
    uint32_t Reserved2;

    enum
    {
      FLAG_VOLUME = 1,
      FLAG_HAS_COMMENT = 2,
      FLAG_BLOCKED = 4,
      FLAG_SOLID = 8,
      FLAG_SIGNATURE = 0x20,
    };
  } PACK_POST;

  // File header is always ExtendedBlockHeader
  PACK_PRE struct FileBlockHeader : ExtendedBlockHeader
  {
    static const uint8_t TYPE = 0x74;

    //CRC from Type to Attributes+
    uint32_t UnpackedSize;
    uint8_t HostOS;
    uint32_t UnpackedCRC;
    uint32_t TimeStamp;
    uint8_t DepackerVersion;
    uint8_t Method;
    uint16_t NameSize;
    uint32_t Attributes;

    enum
    {
      FLAG_SPLIT_BEFORE = 1,
      FLAG_SPLIT_AFTER = 2,
      FLAG_ENCRYPTED = 4,
      FLAG_HAS_COMMENT = 8,
      FLAG_SOLID = 0x10,
      FLAG_DIRECTORY = 0xe0,
      FLAG_BIG_FILE = 0x100,

      MIN_VERSION = 13,
      MAX_VERSION = 20
    };

    bool IsBigFile() const
    {
      return 0 != (fromLE(Flags) & FLAG_BIG_FILE);
    }

    String GetName() const;
  } PACK_POST;

  PACK_PRE struct BigFileBlockHeader : FileBlockHeader
  {
    uint32_t PackedSizeHi;
    uint32_t UnpackedSizeHi;
  } PACK_POST; 

  inline String FileBlockHeader::GetName() const
  {
    const uint8_t* const self = safe_ptr_cast<const uint8_t*>(this);
    const uint8_t* const filename = self + (IsBigFile() ? sizeof(BigFileBlockHeader) : sizeof(FileBlockHeader));
    return String(filename, filename + fromLE(NameSize));
  }

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  class CompressedFile
  {
  public:
    typedef std::auto_ptr<const CompressedFile> Ptr;
    virtual ~CompressedFile() {}

    virtual std::auto_ptr<Dump> Decompress() const = 0;
  };

  class NoCompressedFile : public CompressedFile
  {
  public:
    NoCompressedFile(const uint8_t* const start, std::size_t size, std::size_t destSize)
      : Start(start)
      , Size(size)
      , DestSize(destSize)
    {
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      if (Size != DestSize)
      {
        Log::Debug(THIS_MODULE, "Stored file sizes mismatch");
        return std::auto_ptr<Dump>();
      }
      else
      {
        Log::Debug(THIS_MODULE, "Restore");
        return std::auto_ptr<Dump>(new Dump(Start, Start + DestSize));
      }
    }
  private:
    const uint8_t* const Start;
    const std::size_t Size;
    const std::size_t DestSize;
  };

  class PackedFile : public CompressedFile
  {
  public:
    PackedFile(const uint8_t* const start, std::size_t size, std::size_t destSize)
      : Start(start)
      , Size(size)
      , DestSize(destSize)
    {
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      Log::Debug(THIS_MODULE, "Depack %1% -> %2%", Size, DestSize);
      return std::auto_ptr<Dump>();
    }
  private:
    const uint8_t* const Start;
    const std::size_t Size;
    const std::size_t DestSize;
  };

  class RaredFile : public CompressedFile
  {
  public:
    explicit RaredFile(const FileBlockHeader& header)
    {
      const uint8_t* const start = safe_ptr_cast<const uint8_t*>(&header) + fromLE(header.Size);
      const std::size_t size = fromLE(header.AdditionalSize);
      const std::size_t outSize = fromLE(header.UnpackedSize);
      switch (header.Method)
      {
      case 0x30:
        Delegate.reset(new NoCompressedFile(start, size, outSize));
        break;
      }
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      return Delegate.get()
        ? Delegate->Decompress()
        : std::auto_ptr<Dump>();
    }
  private:
    std::auto_ptr<CompressedFile> Delegate;
  };

  class RarContainerFile : public Container::File
  {
  public:
    RarContainerFile(const String& name, IO::DataContainer::Ptr data)
      : Name(name)
      , Data(data)
      , Header(*static_cast<const FileBlockHeader*>(Data->Data()))
    {
      assert(!Header.IsBigFile());
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetOffset() const
    {
      return ~std::size_t(0);
    }

    virtual std::size_t GetSize() const
    {
      return fromLE(Header.UnpackedSize);
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      Log::Debug(THIS_MODULE, "Decompressing '%1%'", GetName());
      const CompressedFile::Ptr file(new RaredFile(Header));
      std::auto_ptr<Dump> decoded = file->Decompress();
      if (!decoded.get())
      {
        return IO::DataContainer::Ptr();
      }
      return IO::CreateDataContainer(decoded);
    }
  private:
    const String Name;
    const IO::DataContainer::Ptr Data;
    const FileBlockHeader& Header;
  };

  class RarBlocksIterator
  {
  public:
    explicit RarBlocksIterator(IO::DataContainer::Ptr data)
      : Data(data)
      , Limit(Data->Size())
      , Offset(0)
    {
      assert(0 == std::memcmp(Get<BlockHeader>(), &MARKER, sizeof(MARKER)));
    }

    bool IsEof() const
    {
      const uint64_t curBlockSize = GetBlockSize();
      return uint64_t(Offset) + curBlockSize > uint64_t(Limit);
    }

    const BlockHeader& GetCurrentBlock() const
    {
      return *Get<BlockHeader>();
    }

    const ArchiveBlockHeader* GetArchiveHeader() const
    {
      assert(!IsEof());
      const ArchiveBlockHeader& block = *Get<ArchiveBlockHeader>();
      return !block.IsExtended() && ArchiveBlockHeader::TYPE == block.Type
        ? &block
        : 0;
    }

    const FileBlockHeader* GetFileHeader() const
    {
      assert(!IsEof());
      if (const FileBlockHeader* const block = Get<FileBlockHeader>())
      {
        return block->IsExtended() && FileBlockHeader::TYPE == block->Type
          ? block
          : 0;
      }
      return 0;
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }

    uint64_t GetBlockSize() const
    {
      if (const BlockHeader* const block = Get<BlockHeader>())
      {
        uint64_t res = fromLE(block->Size);
        if (block->IsExtended())
        {
          if (const ExtendedBlockHeader* const extBlock = Get<ExtendedBlockHeader>())
          {
            res += fromLE(extBlock->AdditionalSize);
            //Even if big files are not supported, we should properly skip them in stream
            if (FileBlockHeader::TYPE == extBlock->Type && 
                FileBlockHeader::FLAG_BIG_FILE & fromLE(extBlock->Flags))
            {
              if (const BigFileBlockHeader* const bigFile = Get<BigFileBlockHeader>())
              {
                res += uint64_t(fromLE(bigFile->PackedSizeHi)) << (8 * sizeof(uint32_t));
              }
              else
              {
                return sizeof(*bigFile);
              }
            }
          }
          else
          {
            return sizeof(*extBlock);
          }
        }
        return res;
      }
      else
      {
        return sizeof(*block);
      }
    }

    void Next()
    {
      assert(!IsEof());
      Offset += GetBlockSize();
      if (const BlockHeader* block = Get<BlockHeader>())
      {
        //finish block
        if (block->Type == 0x7b && 7 == fromLE(block->Size))
        {
          Offset += sizeof(*block);
        }
      }
    }
  private:
    template<class T>
    const T* Get() const
    {
      if (Limit - Offset >= sizeof(T))
      {
        return safe_ptr_cast<const T*>(static_cast<const uint8_t*>(Data->Data()) + Offset);
      }
      else
      {
        return 0;
      }
    }
  private:
    const IO::DataContainer::Ptr Data;
    const std::size_t Limit;
    std::size_t Offset;
  };

  class RarFilesIterator
  {
  public:
    explicit RarFilesIterator(IO::DataContainer::Ptr data)
      : Data(data)
      , Blocks(data)
    {
      SkipNonFileBlocks();
    }

    bool IsEof() const
    {
      return Blocks.IsEof();
    }

    bool IsValid() const
    {
      assert(!IsEof());
      const FileBlockHeader& file = *Blocks.GetFileHeader();
      const uint_t flags = fromLE(file.Flags);
      //multivolume files are not suported
      if (0 != (flags & (FileBlockHeader::FLAG_SPLIT_BEFORE | FileBlockHeader::FLAG_SPLIT_AFTER)))
      {
        return false;
      }
      //encrypted files are not supported
      if (0 != (flags & FileBlockHeader::FLAG_ENCRYPTED))
      {
        return false;
      }
      //solid files are not supported
      if (0 != (flags & FileBlockHeader::FLAG_SOLID))
      {
        return false;
      }
      //big files are not supported
      if (file.IsBigFile())
      {
        return false;
      }
      //skip directory
      if (FileBlockHeader::FLAG_DIRECTORY == (flags & FileBlockHeader::FLAG_DIRECTORY))
      {
        return false;
      }
      //skip empty files
      if (0 == file.UnpackedSize)
      {
        return false;
      }
      //skip invalid version
      if (!in_range<uint_t>(file.DepackerVersion, FileBlockHeader::MIN_VERSION, FileBlockHeader::MAX_VERSION))
      {
        return false;
      }
      return true;
    }

    String GetName() const
    {
      const FileBlockHeader& file = *Blocks.GetFileHeader();
      String name = file.GetName();
      std::replace(name.begin(), name.end(), '\\', '/');
      return name;
    }

    Container::File::Ptr GetFile() const
    {
      assert(IsValid());
      const std::size_t offset = Blocks.GetOffset();
      const std::size_t size = Blocks.GetBlockSize();
      const IO::DataContainer::Ptr fileBlock = Data->GetSubcontainer(offset, size);
      return boost::make_shared<RarContainerFile>(GetName(), fileBlock);
    }

    void Next()
    {
      assert(!IsEof());
      Blocks.Next();
      SkipNonFileBlocks();
    }

    std::size_t GetOffset() const
    {
      return Blocks.GetOffset();
    }
  private:
    void SkipNonFileBlocks()
    {
      while (!Blocks.IsEof() && !Blocks.GetFileHeader())
      {
        Blocks.Next();
      }
    }
  private:
    const IO::DataContainer::Ptr Data;
    RarBlocksIterator Blocks;
  };

  template<class It>
  class FilesIterator : public Container::Catalogue::Iterator
  {
  public:
    FilesIterator(It begin, It limit)
      : Current(begin)
      , Limit(limit)
    {
    }

    virtual bool IsValid() const
    {
      return Current != Limit;
    }

    virtual Container::File::Ptr Get() const
    {
      assert(IsValid());
      return Current->second;
    }

    virtual void Next()
    {
      assert(IsValid());
      ++Current;
    }
  private:
    It Current;
    const It Limit;
  };

  class RarCatalogue : public Container::Catalogue
  {
  public:
    RarCatalogue(std::size_t maxFileSize, IO::DataContainer::Ptr data)
      : Data(data)
      , MaxFileSize(maxFileSize)
    {
    }

    virtual Iterator::Ptr GetFiles() const
    {
      FillCache();
      return Iterator::Ptr(new FilesIterator<FilesMap::const_iterator>(Files.begin(), Files.end()));
    }

    virtual uint_t GetFilesCount() const
    {
      FillCache();
      return Files.size();
    }

    virtual Container::File::Ptr FindFile(const DataPath& path) const
    {
      const String inPath = path.AsString();
      const String firstComponent = path.GetFirstComponent();
      if (inPath == firstComponent)
      {
        return FindFile(firstComponent);
      }
      Log::Debug(THIS_MODULE, "Opening '%1%'", inPath);
      DataPath::Ptr resolved = CreateDataPath(firstComponent);
      for (;;)
      {
        const String filename = resolved->AsString();
        Log::Debug(THIS_MODULE, "Trying '%1%'", filename);
        if (Container::File::Ptr file = FindFile(filename))
        {
          Log::Debug(THIS_MODULE, "Found");
          return file;
        }
        if (filename == inPath)
        {
          return Container::File::Ptr();
        }
        const DataPath::Ptr unresolved = SubstractDataPath(path, *resolved);
        resolved = CreateMergedDataPath(resolved, unresolved->GetFirstComponent());
      }
    }

    virtual std::size_t GetSize() const
    {
      FillCache();
      assert(Iter->IsEof());
      return Iter->GetOffset();
    }
  private:
    void FillCache() const
    {
      FindNonCachedFile(String());
    }

    Container::File::Ptr FindFile(const String& name) const
    {
      if (Container::File::Ptr file = FindCachedFile(name))
      {
        return file;
      }
      return FindNonCachedFile(name);
    }

    Container::File::Ptr FindCachedFile(const String& name) const
    {
      if (Iter.get())
      {
        const FilesMap::const_iterator it = Files.find(name);
        if (it != Files.end())
        {
          return it->second;
        }
      }
      return Container::File::Ptr();
    }

    Container::File::Ptr FindNonCachedFile(const String& name) const
    {
      CreateIterator();
      while (!Iter->IsEof())
      {
        const String fileName = Iter->GetName();
        if (!Iter->IsValid())
        {
          Log::Debug(THIS_MODULE, "Invalid file '%1%'", fileName);
          Iter->Next();
          continue;
        }
        Log::Debug(THIS_MODULE, "Found file '%1%'", fileName);
        const Container::File::Ptr fileObject = Iter->GetFile();
        Iter->Next();
        if (fileObject->GetSize() > MaxFileSize)
        {
          Log::Debug(THIS_MODULE, "File is too big (%1% bytes)", fileObject->GetSize());
          continue;
        }
        Files.insert(FilesMap::value_type(fileName, fileObject));
        if (fileName == name)
        {
          return fileObject;
        }
      }
      return Container::File::Ptr();
    }

    void CreateIterator() const
    {
      if (!Iter.get())
      {
        Iter.reset(new RarFilesIterator(Data));
      }
    }
  private:
    const IO::DataContainer::Ptr Data;
    const std::size_t MaxFileSize;
    mutable std::auto_ptr<RarFilesIterator> Iter;
    typedef std::map<String, Container::File::Ptr> FilesMap;
    mutable FilesMap Files;
  };

  const std::string RAR_FORMAT(
    //file marker
    "5261"       //uint16_t CRC;   "Ra"
    "72"         //uint8_t Type;   "r"
    "211a"       //uint16_t Flags; "!ESC^"
    "0700"       //uint16_t Size;  "BELL^,0"
    //archive header
    "??"         //uint16_t CRC;
    "73"         //uint8_t Type;
    "%xxxx0xxx?" //uint16_t Flags; skip Solid archives
    "0d00"       //uint16_t Size
  );

  class RarFactory : public ContainerFactory
  {
  public:
    RarFactory()
      : Format(DataFormat::Create(RAR_FORMAT))
    {
    }

    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Container::Catalogue::Ptr CreateContainer(const Parameters::Accessor& parameters, IO::DataContainer::Ptr data) const
    {
      if (!Format->Match(data->Data(), data->Size()))
      {
        return Container::Catalogue::Ptr();
      }
      //TODO: own parameters
      Parameters::IntType maxFileSize = Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB_DEFAULT;
      parameters.FindIntValue(Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB, maxFileSize);
      maxFileSize *= 1 << 20;
      if (maxFileSize > std::numeric_limits<std::size_t>::max())
      {
        maxFileSize = std::numeric_limits<std::size_t>::max();
      }
      return boost::make_shared<RarCatalogue>(static_cast<std::size_t>(maxFileSize), data);
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterRarContainer(PluginsRegistrator& registrator)
  {
    const ContainerFactory::Ptr factory = boost::make_shared<RarFactory>();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
