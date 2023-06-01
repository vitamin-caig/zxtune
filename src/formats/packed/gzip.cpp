/**
 *
 * @file
 *
 * @brief  GZIP compressor support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
// common includes
#include <byteorder.h>
#include <error.h>
#include <make_ptr.h>
// library includes
#include <binary/compression/zlib_stream.h>
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/packed.h>
// std includes
#include <array>

namespace Formats::Packed
{
  namespace Gzip
  {
    using SignatureType = std::array<uint8_t, 2>;

    const SignatureType SIGNATURE = {{0x1f, 0x8b}};

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

    struct Header
    {
      SignatureType Signature;
      uint8_t CompressionMethod;
      uint8_t Flags;
      le_uint32_t ModTime;
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
    };

    struct Footer
    {
      le_uint32_t Crc32;
      le_uint32_t OriginalSize;
    };

    static_assert(sizeof(Header) * alignof(Header) == 10, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(Header) + 2 + sizeof(Footer);

    const Char DESCRIPTION[] = "GZip";
    const auto FORMAT =
        "1f 8b"      // signature
        "08"         // compression method
        "%000xxxxx"  // flags
        "????"       // modtime
        "?"          // extra flags
        "?"          // OS
        ""_sv;
  }  // namespace Gzip

  class GzipDecoder : public Decoder
  {
  public:
    GzipDecoder()
      : Format(Binary::CreateFormat(Gzip::FORMAT, Gzip::MIN_SIZE))
    {}

    String GetDescription() const override
    {
      return Gzip::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Formats::Packed::Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Format->Match(rawData))
      {
        return {};
      }
      try
      {
        Binary::InputStream input(rawData);
        const auto& header = input.Read<Gzip::Header>();
        Require(header.Check());
        if (header.HasExtraData())
        {
          const std::size_t extraSize = input.Read<le_uint16_t>();
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
          const auto& footer = input.Read<Gzip::Footer>();
          Require(result->Size() == footer.OriginalSize);
          // TODO: check CRC
          return CreateContainer(std::move(result), input.GetPosition());
        }
      }
      catch (const Error&)
      {}
      catch (const std::exception&)
      {}
      return {};
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateGzipDecoder()
  {
    return MakePtr<GzipDecoder>();
  }
}  // namespace Formats::Packed
