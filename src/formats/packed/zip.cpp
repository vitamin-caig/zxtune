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
    "%00000xx0 00"  //uint16_t Flags;
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
      return GetUsedSize() <= Size;
    }

    std::size_t GetUsedSize() const
    {
      const Formats::Packed::Zip::LocalFileHeader& header = GetHeader();
      return header.GetTotalFileSize();
    }

    const Formats::Packed::Zip::LocalFileHeader& GetHeader() const
    {
      assert(Size >= sizeof(Formats::Packed::Zip::LocalFileHeader));
      return *safe_ptr_cast<const Formats::Packed::Zip::LocalFileHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
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

  std::auto_ptr<DataDecoder> CreateDecoder(const Formats::Packed::Zip::LocalFileHeader& header)
  {
    const uint8_t* const start = safe_ptr_cast<const uint8_t*>(&header) + header.GetSize();
    const std::size_t size = fromLE(header.Attributes.CompressedSize);
    const std::size_t outSize = fromLE(header.Attributes.UncompressedSize);
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
      : Header(container.GetHeader())
      , Delegate(CreateDecoder(Header))
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
      IsValid = result.get() && result->size() == Header.GetTotalFileSize();
      return result;
    }
  private:
    const Formats::Packed::Zip::LocalFileHeader& Header;
    const std::auto_ptr<DataDecoder> Delegate;
    mutable bool IsValid;
  };
}

namespace Formats
{
  namespace Packed
  {
    namespace Zip
    {
      bool LocalFileHeader::IsValid() const
      {
        return fromLE(Signature) == 0x04034b50;
      }

      std::size_t LocalFileHeader::GetSize() const
      {
        return sizeof(*this) - 1 + fromLE(NameSize) + fromLE(ExtraSize);
      }

      std::size_t LocalFileHeader::GetTotalFileSize() const
      {
        return GetSize() + fromLE(Attributes.CompressedSize);
      }

      bool LocalFileHeader::IsSupported() const
      {
        const uint_t flags = fromLE(Flags);
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
        if (0 == Attributes.UncompressedSize)
        {
          return false;
        }
        return true;
      }
    }

    class ZipDecoder : public Decoder
    {
    public:
      ZipDecoder()
        : Depacker(Binary::Format::Create(::Zip::HEADER_PATTERN))
      {
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        const ::Zip::Container container(data, availSize);
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
        return CreatePackedContainer(decoder.Decompress(), container.GetUsedSize());
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
