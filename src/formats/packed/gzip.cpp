/**
* 
* @file
*
* @brief  GZIP compressor support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
//common includes
#include <byteorder.h>
#include <make_ptr.h>
//library includes
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/packed.h>
#include <math/numeric.h>
//boost includes
#include <boost/array.hpp>
//3rd-party includes
#include <3rdparty/zlib/zlib.h>
//text includes
#include <formats/text/packed.h>

namespace Formats
{
namespace Packed
{
  namespace Gzip
  {
    typedef boost::array<uint8_t, 2> SignatureType;
    
    const SignatureType SIGNATURE = {{ 0x1f, 0x8b }};
    
    enum CompressionMethods
    {
      DEFLATE = 8
    };

    enum FlagBits
    {
      TEXT_MODE = 1,
      HAS_CRC16 = 2,
      HAS_EXTRA = 4,
      HAS_FILENAME = 8,
      HAS_COMMENT = 16
    };
    
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Header
    {
      SignatureType Signature;
      uint8_t CompressionMethod;
      uint8_t Flags;
      uint32_t ModTime;
      uint8_t ExtraFlags;
      uint8_t OSType;
      
      bool Check() const
      {
        return Signature == SIGNATURE && CompressionMethod == DEFLATE;
      }
      
      bool HasExtraData() const
      {
        return 0 != (Flags & HAS_EXTRA);
      }
      
      bool HasFilename() const
      {
        return 0 != (Flags & HAS_FILENAME);
      }
      
      bool HasComment() const
      {
        return 0 != (Flags & HAS_COMMENT);
      }
      
      bool HasCrc16() const
      {
        return 0 != (Flags & HAS_COMMENT);
      }
    } PACK_POST;
    
    PACK_PRE struct Footer
    {
      uint32_t Crc32;
      uint32_t OriginalSize;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(Header) == 10, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header) + 2 + sizeof(Footer);

    const std::string FORMAT(
      "1f 8b" //signature
      "08"    //compression method
      "%000xxxxx" //flags
      "????"  //modtime
      "?"     //extra flags
      "?"     //OS
    );

    void Decompress(Binary::InputStream& input, Binary::DataBuilder& output)
    {
      z_stream stream = z_stream();
      Require(Z_OK == ::inflateInit2(&stream, -15));
      const std::shared_ptr<void> cleanup(&stream, ::inflateEnd);
      for (;;)
      {
        const std::size_t restIn = input.GetRestSize();
        if (stream.avail_in == 0)
        {
          stream.next_in = const_cast<Bytef*>(input.ReadData(0));
          stream.avail_in = static_cast<uInt>(restIn);
        }
        if (stream.avail_out == 0)
        {
          const std::size_t bufSize = Math::Align<std::size_t>(stream.total_in ? uint64_t(stream.avail_in) * stream.total_out / stream.total_in : 1, 16384);
          stream.next_out = static_cast<Bytef*>(output.Allocate(bufSize));
          stream.avail_out = static_cast<uInt>(bufSize);
        }
        const int res = ::inflate(&stream, Z_SYNC_FLUSH);
        if (Z_STREAM_END == res)
        {
          input.ReadData(restIn - stream.avail_in);
          output.Resize(stream.total_out);
          return;
        }
        else if (Z_OK != res)
        { 
          throw std::runtime_error(stream.msg ? stream.msg : "unknown zlib error");
        }
      }
    }
  }//namespace Gzip

  class GzipDecoder : public Decoder
  {
  public:
    GzipDecoder()
      : Format(Binary::CreateFormat(Gzip::FORMAT, Gzip::MIN_SIZE))
    {
    }

    virtual String GetDescription() const
    {
      return Text::GZIP_DECODER_DESCRIPTION;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData) const
    {
      if (!Format->Match(rawData))
      {
        return Formats::Packed::Container::Ptr();
      }
      try
      {
        Binary::InputStream input(rawData);
        const Gzip::Header header = input.ReadField<Gzip::Header>();
        Require(header.Check());
        if (header.HasExtraData())
        {
          const std::size_t extraSize = fromLE(input.ReadField<uint16_t>());
          input.ReadData(extraSize);
        }
        if (header.HasFilename())
        {
          input.ReadCString(input.GetRestSize());
        }
        if (header.HasComment())
        {
          input.ReadCString(input.GetRestSize());
        }
        if (header.HasCrc16())
        {
          input.ReadField<uint16_t>();
        }
        Binary::DataBuilder output;
        Gzip::Decompress(input, output);
        const Binary::Container::Ptr result = output.CaptureResult();
        const Gzip::Footer footer = input.ReadField<Gzip::Footer>();
        Require(result->Size() == fromLE(footer.OriginalSize));
        //TODO: check CRC
        return CreateContainer(result, input.GetPosition());
      }
      catch (const std::exception&)
      {
        return Formats::Packed::Container::Ptr();
      }
    }
  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateGzipDecoder()
  {
    return MakePtr<GzipDecoder>();
  }
}//namespace Packed
}//namespace Formats
