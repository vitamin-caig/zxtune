/**
* 
* @file
*
* @brief  ZIP compressor support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
#include "zip_supp.h"
//common includes
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <binary/format_factories.h>
#include <debug/log.h>
#include <formats/packed.h>
//std includes
#include <cassert>
#include <memory>
//3rd-party includes
#include <3rdparty/zlib/zlib.h>
//text includes
#include <formats/text/packed.h>

namespace Formats
{
namespace Packed
{
  namespace Zip
  {
    const Debug::Stream Dbg("Formats::Packed::Zip");

    //checkers
    const std::string HEADER_PATTERN =
      "504b0304"      //uint32_t Signature;
      "?00"           //uint16_t VersionToExtract;
      "%0000xxx0 %0000x000"  //uint16_t Flags;
      "%0000x00x 00"  //uint16_t CompressionMethod;
    ;

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
        if (Size < sizeof(LocalFileHeader))
        {
          return false;
        }
        const LocalFileHeader& header = GetHeader();
        if (!header.IsValid() || !header.IsSupported())
        {
          return false;
        }
        File = CompressedFile::Create(header, Size);
        if (File.get() && File->GetUnpackedSize())
        {
          return File->GetPackedSize() <= Size;
        }
        return false;
      }

      const LocalFileHeader& GetHeader() const
      {
        assert(Size >= sizeof(LocalFileHeader));
        return *safe_ptr_cast<const LocalFileHeader*>(Data);
      }

      const CompressedFile& GetFile() const
      {
        return *File;
      }
    private:
      const uint8_t* const Data;
      const std::size_t Size;
      mutable std::unique_ptr<const CompressedFile> File;
    };
    
    class DataDecoder
    {
    public:
      virtual ~DataDecoder() {}

      virtual std::unique_ptr<Dump> Decompress() const = 0;
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

      virtual std::unique_ptr<Dump> Decompress() const
      {
        if (Size != DestSize)
        {
          Dbg("Stored file sizes mismatch");
          return std::unique_ptr<Dump>();
        }
        else
        {
          Dbg("Restore %1% bytes", DestSize);
          return std::unique_ptr<Dump>(new Dump(Start, Start + DestSize));
        }
      }
    private:
      const uint8_t* const Start;
      const std::size_t Size;
      const std::size_t DestSize;
    };

    class InflatedDataDecoder : public DataDecoder
    {
    public:
      InflatedDataDecoder(const uint8_t* const start, std::size_t size, std::size_t destSize)
        : Start(start)
        , Size(size)
        , DestSize(destSize)
      {
      }

      virtual std::unique_ptr<Dump> Decompress() const
      {
        Dbg("Inflate %1% -> %2%", Size, DestSize);
        std::unique_ptr<Dump> res(new Dump(DestSize));
        switch (const int err = Uncompress(*res))
        {
        case Z_OK:
          return res;
        case Z_MEM_ERROR:
          Dbg("No memory to deflate");
          break;
        case Z_BUF_ERROR:
          Dbg("No memory in target buffer to deflate");
          break;
        case Z_DATA_ERROR:
          Dbg("Data is corrupted");
          break;
        default:
          Dbg("Unknown error (%1%)", err);
        }
        return std::unique_ptr<Dump>();
      }
    private:
      int Uncompress(Dump& dst) const
      {
        z_stream stream = z_stream();
        int res = ::inflateInit2(&stream, -15);
        if (Z_OK != res)
        {
          return res;
        }
        stream.next_in = const_cast<uint8_t*>(Start);
        stream.avail_in = static_cast<uInt>(Size);
        stream.next_out = &dst[0];
        stream.avail_out = static_cast<uInt>(DestSize);
        res = ::inflate(&stream, Z_FINISH);
        ::inflateEnd(&stream);
        return res == Z_STREAM_END
          ? Z_OK
          : res;
      }
    private:
      const uint8_t* const Start;
      const std::size_t Size;
      const std::size_t DestSize;
    };

    std::unique_ptr<DataDecoder> CreateDecoder(const LocalFileHeader& header, const CompressedFile& file)
    {
      const uint8_t* const start = safe_ptr_cast<const uint8_t*>(&header) + header.GetSize();
      const std::size_t size = file.GetPackedSize() - header.GetSize();
      const std::size_t outSize = file.GetUnpackedSize();
      switch (fromLE(header.CompressionMethod))
      {
      case 0:
        return std::unique_ptr<DataDecoder>(new StoreDataDecoder(start, size, outSize));
        break;
      case 8:
      case 9:
        return std::unique_ptr<DataDecoder>(new InflatedDataDecoder(start, size, outSize));
        break;
      }
      return std::unique_ptr<DataDecoder>();
    }
   
    class DispatchedDataDecoder : public DataDecoder
    {
    public:
      explicit DispatchedDataDecoder(const Container& container)
        : Delegate(CreateDecoder(container.GetHeader(), container.GetFile()))
        , IsValid(container.FastCheck() && Delegate.get())
      {
      }

      virtual std::unique_ptr<Dump> Decompress() const
      {
        if (!IsValid)
        {
          return std::unique_ptr<Dump>();
        }
        std::unique_ptr<Dump> result = Delegate->Decompress();
        IsValid = result.get() != 0;
        return result;
      }
    private:
      const std::unique_ptr<DataDecoder> Delegate;
      mutable bool IsValid;
    };

    class RegularFile : public CompressedFile
    {
    public:
      explicit RegularFile(const LocalFileHeader& header)
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
      const LocalFileHeader& Header;
    };

    class StreamedFile : public CompressedFile
    {
    public:
      StreamedFile(const LocalFileHeader& header, const LocalFileFooter& footer)
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
      const LocalFileHeader& Header;
      const LocalFileFooter& Footer;
    };

    const LocalFileFooter* FindFooter(const LocalFileHeader& header, std::size_t size)
    {
      assert(0 != (fromLE(header.Flags) & FILE_ATTRIBUTES_IN_FOOTER));

      const uint32_t signature = LocalFileFooter::SIGNATURE;
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
        if (offset + sizeof(LocalFileFooter) > size)
        {
          return 0;
        }
        const LocalFileFooter& result = *safe_ptr_cast<const LocalFileFooter*>(found);
        if (fromLE(result.Attributes.CompressedSize) + header.GetSize() == offset)
        {
          return &result;
        }
        seekPos = found + sizeof(signature);
      }
      return 0;
    }

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

    std::unique_ptr<const CompressedFile> CompressedFile::Create(const LocalFileHeader& hdr, std::size_t availSize)
    {
      assert(availSize > sizeof(hdr));
      if (0 != (fromLE(hdr.Flags) & FILE_ATTRIBUTES_IN_FOOTER))
      {
        if (const LocalFileFooter* footer = FindFooter(hdr, availSize))
        {
          return std::unique_ptr<const CompressedFile>(new StreamedFile(hdr, *footer));
        }
        return std::unique_ptr<const CompressedFile>();
      }
      else
      {
        return std::unique_ptr<const CompressedFile>(new RegularFile(hdr));
      }
    }
  }//namespace Zip

  class ZipDecoder : public Decoder
  {
  public:
    ZipDecoder()
      : Depacker(Binary::CreateFormat(Zip::HEADER_PATTERN, sizeof(Zip::LocalFileHeader)))
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

    virtual Container::Ptr Decode(const Binary::Container& rawData) const
    {
      if (!Depacker->Match(rawData))
      {
        return Container::Ptr();
      }
      const Zip::Container container(rawData.Start(), rawData.Size());
      if (!container.FastCheck())
      {
        return Container::Ptr();
      }
      Zip::DispatchedDataDecoder decoder(container);
      return CreateContainer(decoder.Decompress(), container.GetFile().GetPackedSize());
    }
  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateZipDecoder()
  {
    return MakePtr<ZipDecoder>();
  }
}//namespace Packed
}//namespace Formats
