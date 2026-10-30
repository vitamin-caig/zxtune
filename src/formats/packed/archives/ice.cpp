/**
 *
 * @file
 *
 * @brief  ICE! 2.4 compressor support
 *
 * @note   Based on the public domain depacker by Hans Wessels (2007)
 *
 **/

#include "formats/packed/common/container.h"
#include "formats/packed/common/utils.h"

#include "binary/data_builder.h"
#include "binary/format_factories.h"
#include "formats/packed/decoder.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"

#include <algorithm>
#include <exception>

namespace Formats::Packed::ICE
{
  const std::size_t MAX_DECODED_SIZE = 16 * 1024 * 1024;

  const auto DESCRIPTION = "Pack-Ice v2.40"sv;
  const auto HEADER_PATTERN =
      // uint8_t Signature[4]
      "'I'C'E'!"
      // be_uint32_t PackedSize; >= sizeof(RawHeader), <= MAX_DECODED_SIZE
      "00-01???"
      // be_uint32_t UnpackedSize; 1..MAX_DECODED_SIZE
      "00-01???"sv;

  struct RawHeader
  {
    uint8_t Signature[4];
    be_uint32_t PackedSize;
    be_uint32_t UnpackedSize;
  };

  static_assert(sizeof(RawHeader) * alignof(RawHeader) == 12, "Invalid layout");

  const std::size_t MIN_SIZE = sizeof(RawHeader);

  class Parser
  {
  public:
    explicit Parser(Binary::View data)
      : Data(data)
    {}

    bool FastCheck() const
    {
      if (Data.Size() < sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      return header.PackedSize >= sizeof(RawHeader) && header.PackedSize <= MAX_DECODED_SIZE
             && header.PackedSize <= Data.Size() && header.UnpackedSize != 0 && header.UnpackedSize <= MAX_DECODED_SIZE;
    }

    const RawHeader& GetHeader() const
    {
      return *Data.As<RawHeader>();
    }

    std::size_t GetUsedSize() const
    {
      return GetHeader().PackedSize;
    }

    std::size_t GetUnpackedSize() const
    {
      return GetHeader().UnpackedSize;
    }

    Binary::View GetPackedData() const
    {
      return Data.SubView(sizeof(RawHeader), GetUsedSize() - sizeof(RawHeader));
    }

  private:
    const Binary::View Data;
  };

  class Bitstream
  {
  public:
    explicit Bitstream(Binary::View data)
      : Data(data.As<uint8_t>())
      , Position(data.Size())
    {
      // ICE starts from the first set bit in the low half of the last byte; that bit and all
      // lower bits belong to the format's initial reload quirk, not to the data stream.
      Cmd = GetByte();
      Mask = 0x80;
      Require(Cmd != 0);
      while (!(Cmd & 1))
      {
        Cmd >>= 1;
        Mask >>= 1;
      }
      Cmd >>= 1;
    }

    bool GetBit()
    {
      if (!(Mask >>= 1))
      {
        Mask = 0x80;
        Cmd = GetByte();
      }
      return 0 != (Cmd & Mask);
    }

    uint_t GetBits(uint_t count)
    {
      uint_t result = 0;
      while (count--)
      {
        result += result;
        if (GetBit())
        {
          ++result;
        }
      }
      return result;
    }

    uint8_t GetByte()
    {
      Require(Position != 0);
      return Data[--Position];
    }

    std::size_t GetLiteralSize()
    {
      static constexpr struct LiteralSizeEntry
      {
        uint_t Bits;
        uint_t MaxValue;
        uint_t Offset;
      } TABLE[] = {
          {1, 1, 1}, {2, 3, 2}, {2, 3, 5}, {3, 7, 8}, {8, 255, 15}, {15, 32768, 270},
      };
      for (const auto& entry : TABLE)
      {
        const uint_t len = GetBits(entry.Bits);
        if (len != entry.MaxValue)
        {
          return len + entry.Offset;
        }
      }
      throw std::exception();
    }

    std::size_t GetCopyLen()
    {
      if (!GetBit())
      {
        return 0;
      }
      if (!GetBit())
      {
        return 1;
      }
      if (!GetBit())
      {
        return 2 + GetBits(1);
      }
      if (!GetBit())
      {
        return 4 + GetBits(2);
      }
      return 8 + GetBits(10);
    }

    std::size_t GetCopyOffset(std::size_t len)
    {
      if (0 == len)
      {
        return GetBit() ? 64 + GetBits(9) : GetBits(6);
      }
      std::size_t pos;
      if (!GetBit())
      {
        pos = 32 + GetBits(8);
      }
      else if (!GetBit())
      {
        pos = GetBits(5);
      }
      else
      {
        pos = 288 + GetBits(12);
      }
      return pos ? pos + len : 0;
    }

  private:
    const uint8_t* const Data;
    std::size_t Position;
    uint_t Cmd = 0;
    uint_t Mask = 0;
  };

  void CopyLiteral(Bitstream& stream, Binary::DataBuilder& result, std::size_t limit)
  {
    const std::size_t len = std::min(stream.GetLiteralSize(), limit);
    for (std::size_t count = 0; count != len; ++count)
    {
      result.AddByte(stream.GetByte());
    }
  }

  void CopyReference(Bitstream& stream, Binary::DataBuilder& result, std::size_t limit)
  {
    const std::size_t len = stream.GetCopyLen();
    const std::size_t offset = stream.GetCopyOffset(len);
    Require(Formats::Packed::CopyFromBack(offset + 1, result, std::min(len + 2, limit)));
  }

  Binary::Container::Ptr Decode(const Parser& parser)
  {
    Bitstream stream(parser.GetPackedData());
    Binary::DataBuilder result(parser.GetUnpackedSize());
    for (const auto unpacked = parser.GetUnpackedSize(); result.Size() < unpacked;)
    {
      const auto avail = unpacked - result.Size();
      if (stream.GetBit())
      {
        CopyLiteral(stream, result, avail);
      }
      // a reference copy follows every literal (and also appears on its own)
      if (const auto rest = unpacked - result.Size(); rest > 0)
      {
        CopyReference(stream, result, rest);
      }
    }
    Reverse(result);
    return result.CaptureResult();
  }

  class Decoder : public Packed::Decoder
  {
  public:
    StringView GetDescription() const override
    {
      return DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Format->Match(rawData))
      {
        return {};
      }
      const Parser parser(rawData);
      if (!parser.FastCheck())
      {
        return {};
      }
      try
      {
        if (auto decoded = ICE::Decode(parser))
        {
          Require(decoded->Size() == parser.GetUnpackedSize());
          return CreateContainer(std::move(decoded), parser.GetUsedSize());
        }
      }
      catch (const std::exception&)
      {}
      return {};
    }

  private:
    const Binary::Format::Ptr Format = Binary::CreateFormat(HEADER_PATTERN, MIN_SIZE);
  };
}  // namespace Formats::Packed::ICE

namespace Formats::Packed
{
  Decoder::Ptr CreateICEDecoder()
  {
    return MakePtr<ICE::Decoder>();
  }
}  // namespace Formats::Packed
