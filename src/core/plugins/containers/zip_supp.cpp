/*
Abstract:
  Zip convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef ZLIB_SUPPORT
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

  const Char ID[] = {'Z', 'I', 'P', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::ZIP_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK;

  const std::string THIS_MODULE("Core::ZIPSupp");

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct FileAttributes
  {
    uint32_t CRC;
    uint32_t CompressedSize;
    uint32_t UncompressedSize;
  } PACK_POST;

  enum
  {
    FILE_CRYPTED = 0x1,
    FILE_COMPRESSION_LEVEL_MASK = 0x6,
    FILE_COMPRESSION_LEVEL_NONE = 0x0,
    FILE_COMPRESSION_LEVEL_NORMAL = 0x2,
    FILE_COMPRESSION_LEVEL_FAST = 0x4,
    FILE_COMPRESSION_LEVEL_EXTRAFAST = 0x6,
    FILE_ATTRIBUTES_IN_FOOTER = 0x8,
    FILE_UTF8 = 0x800
  };

  PACK_PRE struct LocalFileHeader
  {
    //+0
    uint32_t Signature; //0x04034b50
    //+4
    uint16_t VersionToExtract;
    //+6
    uint16_t Flags;
    //+8
    uint16_t CompressionMethod;
    //+a
    uint16_t ModificationTime;
    //+c
    uint16_t ModificationDate;
    //+e
    FileAttributes Attributes;
    //+1a
    uint16_t NameSize;
    //+1c
    uint16_t ExtraSize;
    //+1e
    uint8_t Name[1];


    bool IsValid() const
    {
      return fromLE(Signature) == 0x04034b50;
    }

    std::size_t GetSize() const
    {
      return sizeof(*this) - 1 + fromLE(NameSize) + fromLE(ExtraSize);
    }

    std::size_t GetTotalFileSize() const
    {
      return GetSize() + fromLE(Attributes.CompressedSize);
    }
  } PACK_POST;

  PACK_PRE struct LocalFileFooter
  {
    //+0
    uint32_t Signature; //0x08074b50
    //+4
    FileAttributes Attributes;
  } PACK_POST;
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

  class InflatedFile : public CompressedFile
  {
  public:
    InflatedFile(const uint8_t* const start, std::size_t size, std::size_t destSize)
      : Start(start)
      , Size(size)
      , DestSize(destSize)
    {
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      Log::Debug(THIS_MODULE, "Inflate %1% -> %2%", Size, DestSize);
      std::auto_ptr<Dump> res(new Dump(DestSize));
      switch (const int err = Uncompress(*res))
      {
      case Z_OK:
        return res;
      case Z_MEM_ERROR:
        Log::Debug(THIS_MODULE, "No memory to deflate");
        break;
      case Z_BUF_ERROR:
        Log::Debug(THIS_MODULE, "No memory in target buffer to deflate");
        break;
      case Z_DATA_ERROR:
        Log::Debug(THIS_MODULE, "Data is corrupted");
        break;
      default:
        Log::Debug(THIS_MODULE, "Unknown error (%1%)", err);
      }
      return std::auto_ptr<Dump>();
    }
  private:
    int Uncompress(Dump& dst) const
    {
      z_stream stream = z_stream();
      int res = inflateInit2(&stream, -15);
      if (Z_OK != res)
      {
        return res;
      }
      stream.next_in = const_cast<uint8_t*>(Start);
      stream.avail_in = Size;
      stream.next_out = &dst[0];
      stream.avail_out = DestSize;
      res = inflate(&stream, Z_FINISH);
      inflateEnd(&stream);
      return res == Z_STREAM_END
        ? Z_OK
        : res;
    }
  private:
    const uint8_t* const Start;
    const std::size_t Size;
    const std::size_t DestSize;
  };

  class ZippedFile : public CompressedFile
  {
  public:
    explicit ZippedFile(const LocalFileHeader& header)
    {
      const uint8_t* const start = safe_ptr_cast<const uint8_t*>(&header) + header.GetSize();
      const std::size_t size = fromLE(header.Attributes.CompressedSize);
      const std::size_t outSize = fromLE(header.Attributes.UncompressedSize);
      switch (fromLE(header.CompressionMethod))
      {
      case 0:
        Delegate.reset(new NoCompressedFile(start, size, outSize));
        break;
      case 8:
      case 9:
        Delegate.reset(new InflatedFile(start, size, outSize));
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

  class ZIPContainerFile : public Container::File
  {
  public:
    explicit ZIPContainerFile(IO::DataContainer::Ptr data)
      : Data(data)
      , Header(*static_cast<const LocalFileHeader*>(Data->Data()))
    {
      assert(Data->Size() >= Header.GetTotalFileSize());
    }

    virtual String GetName() const
    {
      return String(Header.Name, Header.Name + fromLE(Header.NameSize));
    }

    virtual std::size_t GetOffset() const
    {
      return ~std::size_t(0);
    }

    virtual std::size_t GetSize() const
    {
      return fromLE(Header.Attributes.UncompressedSize);
    }

    virtual ZXTune::IO::DataContainer::Ptr GetData() const
    {
      Log::Debug(THIS_MODULE, "Decompressing '%1%'", GetName());
      const CompressedFile::Ptr file(new ZippedFile(Header));
      std::auto_ptr<Dump> decoded = file->Decompress();
      if (!decoded.get())
      {
        return IO::DataContainer::Ptr();
      }
      return IO::CreateDataContainer(decoded);
    }
  private:
    const IO::DataContainer::Ptr Data;
    const LocalFileHeader& Header;
  };

  class ZipIterator
  {
  public:
    explicit ZipIterator(IO::DataContainer::Ptr data)
      : Data(data)
      , Limit(Data->Size())
      , Offset(0)
    {
    }

    bool IsEof() const
    {
      const std::size_t dataRest = Limit - Offset;
      if (dataRest < sizeof(LocalFileHeader))
      {
        return true;
      }
      const LocalFileHeader& header = GetHeader();
      if (!header.IsValid())
      {
        return true;
      }
      if (dataRest < header.GetTotalFileSize())
      {
        return true;
      }
      return false;
    }

    bool IsValid() const
    {
      assert(!IsEof());
      const LocalFileHeader& header = GetHeader();
      const uint_t flags = fromLE(header.Flags);
      if (0 != (flags & FILE_CRYPTED))
      {
        return false;
      }
      if (0 != (flags & FILE_ATTRIBUTES_IN_FOOTER))
      {
        return false;
      }
      if (0 != (flags & FILE_UTF8))
      {
        return false;
      }
      if (0 == header.Attributes.UncompressedSize)
      {
        return false;
      }
      return true;
    }

    String GetName() const
    {
      assert(!IsEof());
      const LocalFileHeader& header = GetHeader();
      return String(header.Name, header.Name + fromLE(header.NameSize));
    }

    Container::File::Ptr GetFile() const
    {
      assert(IsValid());
      const std::size_t size = GetHeader().GetTotalFileSize();
      return boost::make_shared<ZIPContainerFile>(Data->GetSubcontainer(Offset, size));
    }

    void Next()
    {
      assert(!IsEof());
      Offset += GetHeader().GetTotalFileSize();
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }
  private:
    const LocalFileHeader& GetHeader() const
    {
      assert(Limit - Offset >= sizeof(LocalFileHeader));
      return *safe_ptr_cast<const LocalFileHeader*>(static_cast<const uint8_t*>(Data->Data()) + Offset);
    }
  private:
    const IO::DataContainer::Ptr Data;
    const std::size_t Limit;
    std::size_t Offset;
  };

  template<class It>
  class ZipFilesIterator : public Container::Catalogue::Iterator
  {
  public:
    ZipFilesIterator(It begin, It limit)
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

  class ZipCatalogue : public Container::Catalogue
  {
  public:
    ZipCatalogue(std::size_t maxFileSize, IO::DataContainer::Ptr data)
      : Data(data)
      , MaxFileSize(maxFileSize)
    {
    }

    virtual Iterator::Ptr GetFiles() const
    {
      FillCache();
      return Iterator::Ptr(new ZipFilesIterator<FilesMap::const_iterator>(Files.begin(), Files.end()));
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
        Iter.reset(new ZipIterator(Data));
      }
    }
  private:
    const IO::DataContainer::Ptr Data;
    const std::size_t MaxFileSize;
    mutable std::auto_ptr<ZipIterator> Iter;
    typedef std::map<String, Container::File::Ptr> FilesMap;
    mutable FilesMap Files;
  };

  const std::string ZIP_FORMAT(
    "504b0304"      //uint32_t Signature;
    "?00"           //uint16_t VersionToExtract;
    "%00000xx0 00"  //uint16_t Flags;
    "%0000x00x 00"  //uint16_t CompressionMethod;
  );

  class ZipFactory : public ContainerFactory
  {
  public:
    ZipFactory()
      : Format(DataFormat::Create(ZIP_FORMAT))
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
      Parameters::IntType maxFileSize = Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB_DEFAULT;
      parameters.FindIntValue(Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB, maxFileSize);
      maxFileSize *= 1 << 20;
      if (maxFileSize > std::numeric_limits<std::size_t>::max())
      {
        maxFileSize = std::numeric_limits<std::size_t>::max();
      }
      return boost::make_shared<ZipCatalogue>(static_cast<std::size_t>(maxFileSize), data);
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterZipContainer(PluginsRegistrator& registrator)
  {
    const ContainerFactory::Ptr factory = boost::make_shared<ZipFactory>();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
#else
namespace ZXTune
{
  void RegisterZipContainer(PluginsRegistrator& /*registrator*/)
  {
  }
}
#endif