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
#include "core/plugins/registrator.h"
#include "core/plugins/containers/trdos_process.h"
//common includes
#include <byteorder.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed_decoders.h>
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
        return std::auto_ptr<Dump>(new Dump(Start, Start + DestSize));
      }
    }
  private:
    const uint8_t* const Start;
    const std::size_t Size;
    const std::size_t DestSize;
  };

  int Uncompress(const uint8_t* src, std::size_t srcSize, uint8_t* dst, std::size_t dstSize)
  {
    z_stream stream = z_stream();
    int res = inflateInit2(&stream, -15);
    if (Z_OK != res)
    {
      return res;
    }
    stream.next_in = const_cast<uint8_t*>(src);
    stream.avail_in = srcSize;
    stream.next_out = dst;
    stream.avail_out = dstSize;
    res = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    return res == Z_STREAM_END
      ? Z_OK
      : res;
  }

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
      Log::Debug(THIS_MODULE, "Decompressing %1% -> %2%", Size, DestSize);
      std::auto_ptr<Dump> res(new Dump(DestSize));
      switch (const int err = Uncompress(Start, Size, &res->front(), DestSize))
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

  class FileIterator
  {
  public:
    explicit FileIterator(IO::DataContainer::Ptr data)
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
      if (dataRest < header.GetSize())
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

    std::size_t GetArchivedSize() const
    {
      assert(!IsEof());
      const LocalFileHeader& header = GetHeader();
      assert(0 == (fromLE(header.Flags) & FILE_ATTRIBUTES_IN_FOOTER));
      return header.GetSize() + fromLE(header.Attributes.CompressedSize);
    }

    std::size_t GetUncompressedSize() const
    {
      assert(!IsEof());
      const LocalFileHeader& header = GetHeader();
      assert(0 == (fromLE(header.Flags) & FILE_ATTRIBUTES_IN_FOOTER));
      return fromLE(header.Attributes.UncompressedSize);
    }

    IO::DataContainer::Ptr Extract() const
    {
      assert(IsValid());
      const LocalFileHeader& header = GetHeader();
      assert(0 == (fromLE(header.Flags) & FILE_ATTRIBUTES_IN_FOOTER));
      const CompressedFile::Ptr file(new ZippedFile(header));
      std::auto_ptr<Dump> decoded = file->Decompress();
      if (!decoded.get())
      {
        return IO::DataContainer::Ptr();
      }
      return IO::CreateDataContainer(decoded);
    }

    void Next()
    {
      assert(!IsEof());
      Offset += GetArchivedSize();
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }
  private:
    const LocalFileHeader& GetHeader() const
    {
      assert(Limit - Offset > sizeof(LocalFileHeader));
      return *safe_ptr_cast<const LocalFileHeader*>(static_cast<const uint8_t*>(Data->Data()) + Offset);
    }
  private:
    const IO::DataContainer::Ptr Data;
    const std::size_t Limit;
    std::size_t Offset;
  };

  TRDos::Catalogue::Ptr ParseZipFile(IO::DataContainer::Ptr data)
  {
    TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateGeneric();
    FileIterator iter(data);
    for (; !iter.IsEof(); iter.Next())
    {
      const String fileName = iter.GetName();
      if (!iter.IsValid())
      {
        Log::Debug(THIS_MODULE, "Invalid file '%1%'", fileName);
        continue;
      }
      const std::size_t fileSize = iter.GetUncompressedSize();
      Log::Debug(THIS_MODULE, "Found file '%1%'", fileName);
      const IO::DataContainer::Ptr fileData = iter.Extract();
      const TRDos::File::Ptr file = TRDos::File::Create(fileData, fileName, ~std::size_t(0), fileSize);
      builder->AddFile(file);
    }
    builder->SetUsedSize(iter.GetOffset());
    return builder->GetResult();
  }

  const std::string ZIP_FORMAT(
    "504b0304" //uint32_t Signature;
    "?00"     // uint16_t VersionToExtract;
    "%00000xx0 00"          // uint16_t Flags;
    "%0000x00x"     //CompressionMethod;
  );

  class ZipPlugin : public ArchivePlugin
  {
  public:
    ZipPlugin()
      : Description(CreatePluginDescription(ID, INFO, VERSION, CAPS))
      , Format(DataFormat::Create(ZIP_FORMAT))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr rawData = input->GetData();
      if (const TRDos::Catalogue::Ptr files = ParseZipFile(rawData))
      {
        if (files->GetFilesCount())
        {
          ProcessEntries(input, callback, Description, *files);
          return DetectionResult::CreateMatched(files->GetUsedSize());
        }
      }
      return DetectionResult::CreateUnmatched(Format, rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*commonParams*/, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const String& pathComp = inPath.GetFirstComponent();
      if (pathComp.empty())
      {
        return DataLocation::Ptr();
      }
      const IO::DataContainer::Ptr inData = location->GetData();
      if (const TRDos::Catalogue::Ptr files = ParseZipFile(inData))
      {
        if (const TRDos::File::Ptr fileToOpen = files->FindFile(pathComp))
        {
          if (const IO::DataContainer::Ptr subData = fileToOpen->GetData())
          {
            return CreateNestedLocation(location, subData, Description, pathComp); 
          }
        }
      }
      return DataLocation::Ptr();
    }
  private:
    const Plugin::Ptr Description;
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterZipContainer(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new ZipPlugin());
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