/**
 *
 * @file
 *
 * @brief  LaserCompact v5.2 support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/image/container.h"

#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "formats/image.h"

#include "contract.h"
#include "make_ptr.h"

#include <memory>

namespace Formats::Image
{
  namespace LaserCompact52
  {
    struct Header
    {
      uint8_t Signature[5];
      le_uint16_t PackedSize;  // starting from SizeCode, may be invalid
      uint8_t AdditionalSize;
    };

    static_assert(sizeof(Header) * alignof(Header) == 8, "Invalid layout");

    const std::size_t MIN_SIZE = 16;

    const std::size_t PIXELS_SIZE = 6144;
    const std::size_t ATTRS_SIZE = 768;

    class BitStream
    {
    public:
      explicit BitStream(Binary::DataInputStream& stream)
        : Stream(stream)
      {}

      uint8_t GetByte()
      {
        return Stream.ReadByte();
      }

      uint8_t GetBit()
      {
        if (0 == Mask)
        {
          Bits = GetByte();
          Mask = 0x80;
        }
        const uint8_t res = 0 != (Bits & Mask) ? 1 : 0;
        Mask >>= 1;
        return res;
      }

      uint8_t GetCode()
      {
        uint8_t res = 0xfe;
        for (uint_t i = 0; i < 3; ++i)
        {
          if (GetBit())
          {
            return res + 1;
          }
          res = 2 * res + GetBit();
        }
        return (2 * res + GetBit()) + 9;
      }

      uint8_t GetLen()
      {
        const uint8_t len = GetCode();
        if (len == 0x100 - 7)
        {
          return GetByte() - 1;
        }
        else if (len > 0x100 - 7)
        {
          return len - 1;
        }
        else
        {
          return len;
        }
      }

    private:
      Binary::DataInputStream& Stream;
      uint_t Bits = 0;
      uint_t Mask = 0;
    };

    class AddrTranslator
    {
    public:
      explicit AddrTranslator(uint_t sizeCode)
        : AttrBase(256 * sizeCode)
        , ScrStart((AttrBase & 0x0300) << 3)
        , ScrLimit((AttrBase ^ 0x1800) & 0xfc00)
      {
        Require(sizeCode == 0 || sizeCode == 1 || sizeCode == 2 || sizeCode == 8 || sizeCode == 9 || sizeCode == 16);
      }

      std::size_t GetStart() const
      {
        return ScrStart;
      }

      std::size_t operator()(std::size_t virtAddr) const
      {
        if (virtAddr < ScrLimit)
        {
          const std::size_t line = (virtAddr & 0x0007) << 8;
          const std::size_t row = (virtAddr & 0x0038) << 2;
          const std::size_t col = (virtAddr & 0x07c0) >> 6;
          return (virtAddr & 0x1800) | line | row | col;
        }
        else
        {
          return AttrBase + virtAddr;
        }
      }

    private:
      const std::size_t AttrBase;
      const std::size_t ScrStart;
      const std::size_t ScrLimit;
    };

    Binary::Dump Decode(Binary::DataInputStream& stream)
    {
      try
      {
        const auto& header = stream.Read<Header>();
        stream.Skip(header.AdditionalSize);
        BitStream bitstream(stream);

        Binary::Dump decoded(PIXELS_SIZE + ATTRS_SIZE);
        std::fill_n(&decoded[PIXELS_SIZE], ATTRS_SIZE, 7);

        const AddrTranslator translate(bitstream.GetByte());

        std::size_t target = translate.GetStart();
        decoded.at(translate(target++)) = bitstream.GetByte();
        for (;;)
        {
          if (bitstream.GetBit())
          {
            decoded.at(translate(target++)) = bitstream.GetByte();
          }
          else
          {
            uint8_t len = bitstream.GetLen();
            if (0xff == len)
            {
              break;
            }
            uint16_t dist = bitstream.GetCode() << 8;
            const int_t step = bitstream.GetBit() ? -1 : +1;
            dist |= bitstream.GetByte();

            len = -len;
            dist = -dist;
            if (dist > 768)
            {
              ++len;
            }
            uint16_t from = target - dist;
            do
            {
              decoded.at(translate(target++)) = decoded.at(translate(from));
              from += step;
            } while (--len > 0);
          }
        }
        if (target <= PIXELS_SIZE)
        {
          decoded.resize(PIXELS_SIZE);
        }
        return decoded;
      }
      catch (const std::exception&)
      {
        return {};
      }
    }

    const auto DESCRIPTION = "LaserCompact 5.2"sv;
    const auto FORMAT =
        // Signature
        "'L'C'M'P'5"
        ""sv;
  }  // namespace LaserCompact52

  class LaserCompact52Decoder : public Decoder
  {
  public:
    LaserCompact52Decoder()
      : Format(Binary::CreateFormat(LaserCompact52::FORMAT, LaserCompact52::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return LaserCompact52::DESCRIPTION;
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
      Binary::DataInputStream stream(data);
      auto result = LaserCompact52::Decode(stream);
      return CreateContainer(std::move(result), stream.GetPosition());
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateLaserCompact52Decoder()
  {
    return MakePtr<LaserCompact52Decoder>();
  }
}  // namespace Formats::Image
