/*
Abstract:
  Zip support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "container.h"
#include "zip_supp.h"
//common includes
#include <logging.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <cassert>
#include <memory>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/packed.h>

#ifdef ZLIB_SUPPORT
//platform includes
#include <zlib.h>
#endif

namespace Zip
{
  //checkers
  const std::string HEADER_PATTERN =
    "504b0304"      //uint32_t Signature;
    "?00"           //uint16_t VersionToExtract;
    "%0000xxx0 %0000x000"  //uint16_t Flags;
    "%0000x00x 00"  //uint16_t CompressionMethod;
  ;

  const std::string THIS_MODULE("Formats::Packed::Zip");

  class Container
  {
  public:
    Container(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data))
      , Size(size)
    {
    }

    bool FastCheck() const
    {
      if (Size < sizeof(Formats::Packed::Zip::LocalFileHeader))
      {
        return false;
      }
      const Formats::Packed::Zip::LocalFileHeader& header = GetHeader();
      if (!header.IsValid() || !header.IsSupported())
      {
        return false;
      }
      File = Formats::Packed::Zip::CompressedFile::Create(header, Size);
      if (File.get() && File->GetUnpackedSize())
      {
        return File->GetPackedSize() <= Size;
      }
      return false;
    }

    const Formats::Packed::Zip::LocalFileHeader& GetHeader() const
    {
      assert(Size >= sizeof(Formats::Packed::Zip::LocalFileHeader));
      return *safe_ptr_cast<const Formats::Packed::Zip::LocalFileHeader*>(Data);
    }

    const Formats::Packed::Zip::CompressedFile& GetFile() const
    {
      return *File;
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
    mutable std::auto_ptr<const Formats::Packed::Zip::CompressedFile> File;
  };
  
  class DataDecoder
  {
  public:
    virtual ~DataDecoder() {}

    virtual std::auto_ptr<Dump> Decompress() const = 0;
  };

  class StoreDataDecoder : public DataDecoder
  {
  public:
    StoreDataDecoder(const uint8_t* const start, std::size_t size, std::size_t destSize)
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
        Log::Debug(THIS_MODULE, "Restore %1% bytes", DestSize);
        return std::auto_ptr<Dump>(new Dump(Start, Start + DestSize));
      }
    }
  private:
    const uint8_t* const Start;
    const std::size_t Size;
    const std::size_t DestSize;
  };

#ifdef ZLIB_SUPPORT
  class InflatedDataDecoder : public DataDecoder
  {
  public:
    InflatedDataDecoder(const uint8_t* const start, std::size_t size, std::size_t destSize)
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
#endif

  std::auto_ptr<DataDecoder> CreateDecoder(const Formats::Packed::Zip::LocalFileHeader& header, const Formats::Packed::Zip::CompressedFile& file)
  {
    const uint8_t* const start = safe_ptr_cast<const uint8_t*>(&header) + header.GetSize();
    const std::size_t size = file.GetPackedSize() - header.GetSize();
    const std::size_t outSize = file.GetUnpackedSize();
    switch (fromLE(header.CompressionMethod))
    {
    case 0:
      return std::auto_ptr<DataDecoder>(new StoreDataDecoder(start, size, outSize));
      break;
    case 8:
    case 9:
#ifdef ZLIB_SUPPORT
      return std::auto_ptr<DataDecoder>(new InflatedDataDecoder(start, size, outSize));
#endif
      break;
    }
    return std::auto_ptr<DataDecoder>();
  }
 
  class DispatchedDataDecoder : public DataDecoder
  {
  public:
    explicit DispatchedDataDecoder(const Container& container)
      : Delegate(CreateDecoder(container.GetHeader(), container.GetFile()))
      , IsValid(container.FastCheck() && Delegate.get())
    {
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      if (!IsValid)
      {
        return std::auto_ptr<Dump>();
      }
      std::auto_ptr<Dump> result = Delegate->Decompress();
      IsValid = result.get() != 0;
      return result;
    }
  private:
    const std::auto_ptr<DataDecoder> Delegate;
    mutable bool IsValid;
  };

  class RegularFile : public Formats::Packed::Zip::CompressedFile
  {
  public:
    explicit RegularFile(const Formats::Packed::Zip::LocalFileHeader& header)
      : Header(header)
    {
    }

    virtual std::size_t GetPackedSize() const
    {
      return Header.GetSize() + fromLE(Header.Attributes.CompressedSize);
    }

    virtual std::size_t GetUnpackedSize() const
    {
      return fromLE(Header.Attributes.UncompressedSize);
    }
  private:
    const Formats::Packed::Zip::LocalFileHeader& Header;
  };

  class StreamedFile : public Formats::Packed::Zip::CompressedFile
  {
  public:
    StreamedFile(const Formats::Packed::Zip::LocalFileHeader& header, const Formats::Packed::Zip::LocalFileFooter& footer)
      : Header(header)
      , Footer(footer)
    {
    }

    virtual std::size_t GetPackedSize() const
    {
      return Header.GetSize() + fromLE(Footer.Attributes.CompressedSize) + sizeof(Footer);
    }

    virtual std::size_t GetUnpackedSize() const
    {
      return fromLE(Footer.Attributes.UncompressedSize);
    }
  private:
    const Formats::Packed::Zip::LocalFileHeader& Header;
    const Formats::Packed::Zip::LocalFileFooter& Footer;
  };

  const Formats::Packed::Zip::LocalFileFooter* FindFooter(const Formats::Packed::Zip::LocalFileHeader& header, std::size_t size)
  {
    assert(0 != (fromLE(header.Flags) & Formats::Packed::Zip::FILE_ATTRIBUTES_IN_FOOTER));

    const uint32_t signature = Formats::Packed::Zip::LocalFileFooter::SIGNATURE;
    const uint8_t* const rawSignature = safe_ptr_cast<const uint8_t*>(&signature);

    const uint8_t* const seekStart = safe_ptr_cast<const uint8_t*>(&header);
    const uint8_t* const seekEnd = seekStart + size;
    for (const uint8_t* seekPos = seekStart; seekPos < seekEnd; )
    {
      const uint8_t* const found = std::search(seekPos, seekEnd, rawSignature, rawSignature + sizeof(signature));
      if (found == seekEnd)
      {
        return 0;
      }
      const std::size_t offset = found - seekStart;
      if (offset + sizeof(Formats::Packed::Zip::LocalFileFooter) > size)
      {
        return 0;
      }
      const Formats::Packed::Zip::LocalFileFooter& result = *safe_ptr_cast<const Formats::Packed::Zip::LocalFileFooter*>(found);
      if (fromLE(result.Attributes.CompressedSize) + header.GetSize() == offset)
      {
        return &result;
      }
      seekPos = found + sizeof(signature);
    }
    return 0;
  }
}

namespace Formats
{
  namespace Packed
  {
    namespace Zip
    {
      bool LocalFileHeader::IsValid() const
      {
        return fromLE(Signature) == SIGNATURE;
      }

      std::size_t LocalFileHeader::GetSize() const
      {
        return sizeof(*this) - 1 + fromLE(NameSize) + fromLE(ExtraSize);
      }

      bool LocalFileHeader::IsSupported() const
      {
        const uint_t flags = fromLE(Flags);
        if (0 != (flags & FILE_CRYPTED))
        {
          return false;
        }
        return true;
      }

      std::size_t ExtraDataRecord::GetSize() const
      {
        return sizeof(*this) - 1 + fromLE(DataSize);
      }

      std::size_t CentralDirectoryFileHeader::GetSize() const
      {
        return sizeof(*this) - 1 + fromLE(NameSize) + fromLE(ExtraSize) + fromLE(CommentSize);
      }

      std::size_t CentralDirectoryEnd::GetSize() const
      {
        return sizeof(*this) + fromLE(CommentSize);
      }

      std::size_t DigitalSignature::GetSize() const
      {
        return sizeof(*this) - 1 + fromLE(DataSize);
      }

      std::auto_ptr<const CompressedFile> CompressedFile::Create(const LocalFileHeader& hdr, std::size_t availSize)
      {
        assert(availSize > sizeof(hdr));
        if (0 != (fromLE(hdr.Flags) & FILE_ATTRIBUTES_IN_FOOTER))
        {
          if (const LocalFileFooter* footer = ::Zip::FindFooter(hdr, availSize))
          {
            return std::auto_ptr<const CompressedFile>(new ::Zip::StreamedFile(hdr, *footer));
          }
          return std::auto_ptr<const CompressedFile>();
        }
        else
        {
          return std::auto_ptr<const CompressedFile>(new ::Zip::RegularFile(hdr));
        }
      }
    }

    class ZipDecoder : public Decoder
    {
    public:
      ZipDecoder()
        : Depacker(Binary::Format::Create(::Zip::HEADER_PATTERN))
      {
      }

      virtual String GetDescription() const
      {
        return Text::ZIP_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        return Depacker->Match(data, availSize);
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        const ::Zip::Container container(data, availSize);
        if (!container.FastCheck() || !Depacker->Match(data, availSize))
        {
          return Container::Ptr();
        }
        ::Zip::DispatchedDataDecoder decoder(container);
        return CreatePackedContainer(decoder.Decompress(), container.GetFile().GetPackedSize());
      }
    private:
      const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreateZipDecoder()
    {
      return boost::make_shared<ZipDecoder>();
    }
  }
}
