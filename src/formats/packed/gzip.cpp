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
#include "formats/packed/container.h"
//common includes
#include <byteorder.h>
#include <error.h>
#include <make_ptr.h>
//library includes
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <binary/compression/zlib_stream.h>
#include <formats/packed.h>
//std includes
#include <array>
//text includes
#include <formats/text/packed.h>

namespace Formats
{
namespace Packed
{
  namespace Gzip
  {
    typedef std::array<uint8_t, 2> SignatureType;
    
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

    const StringView FORMAT(
      "1f 8b" //signature
      "08"    //compression method
      "%000xxxxx" //flags
      "????"  //modtime
      "?"     //extra flags
      "?"     //OS
    );
  }//namespace Gzip

  class GzipDecoder : public Decoder
  {
  public:
    GzipDecoder()
      : Format(Binary::CreateFormat(Gzip::FORMAT, Gzip::MIN_SIZE))
    {
    }

    String GetDescription() const override
    {
      return Text::GZIP_DECODER_DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData) const override
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
          const auto extraSize = input.ReadLE<uint16_t>();
          input.Skip(extraSize);
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
          input.Skip(sizeof(uint16_t));
        }
        Binary::DataBuilder output;
        Binary::Compression::Zlib::DecompressRaw(input, output);
        if (auto result = output.CaptureResult())
        {
          const Gzip::Footer footer = input.ReadField<Gzip::Footer>();
          Require(result->Size() == fromLE(footer.OriginalSize));
          //TODO: check CRC
          return CreateContainer(std::move(result), input.GetPosition());
        }
      }
      catch (const Error&)
      {
      }
      catch (const std::exception&)
      {
      }
      return Formats::Packed::Container::Ptr();
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
