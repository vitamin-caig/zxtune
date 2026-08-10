/**
 *
 * @file
 *
 * @brief  MicrospacePacker packer support
 *
 * @author vitamin.caig@gmail.com
 *
 * @note   Based on XLook sources by HalfElf
 *
 **/

#include "formats/packed/common/container.h"
#include "formats/packed/common/utils.h"
#include "formats/packed/zx/hrust1.h"

#include "binary/format_factories.h"
#include "formats/packed/decoder.h"
#include "math/numeric.h"

#include "byteorder.h"
#include "make_ptr.h"
#include "pointers.h"

#include <algorithm>
#include <cstring>
#include <iterator>

namespace Formats::Packed
{
  namespace MSPack
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    const auto DESCRIPTION = "MicroSpace Packer v1.x"sv;
    const auto DEPACKER_PATTERN = "'M's'P'k"sv;

    struct RawHeader
    {
      //+0
      char Signature[4];  //'M','s','P','k'
      //+4
      le_uint16_t LastSrcRestBytes;
      //+6
      le_uint16_t SrcPacked;
      //+8
      le_uint16_t DstPacked;
      //+a
      le_uint16_t SizeOfPacked;  // full data size starting from next field excluding last 5 bytes
      //+c
      le_uint16_t DstAddress;
      //+e
      uint8_t BitStream[2];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x10, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(RawHeader);

    const std::size_t LAST_BYTES_COUNT = 5;

    class Parser
    {
    public:
      explicit Parser(Binary::View data)
        : Data(data)
      {}

      bool FastCheck() const
      {
        if (Data.Size() <= sizeof(RawHeader))
        {
          return false;
        }
        const RawHeader& header = GetHeader();
        const uint_t endOfBlock = header.LastSrcRestBytes + 1;
        const uint_t lastBytesAddr = endOfBlock - LAST_BYTES_COUNT;
        const uint_t bitStreamAddr = lastBytesAddr - header.SizeOfPacked;

        const uint_t srcPacked = header.SrcPacked;
        if (bitStreamAddr != srcPacked &&    // move forward
            lastBytesAddr != srcPacked + 1)  // move backward
        {
          return false;
        }
        const std::size_t usedSize = GetUsedSize();
        return Math::InRange(usedSize, sizeof(header), Data.Size());
      }

      uint_t GetUsedSize() const
      {
        const RawHeader& header = GetHeader();
        return offsetof(RawHeader, DstAddress) + header.SizeOfPacked + LAST_BYTES_COUNT;
      }

      const RawHeader& GetHeader() const
      {
        return *Data.As<RawHeader>();
      }

    private:
      const Binary::View Data;
    };

    class DataDecoder
    {
    public:
      explicit DataDecoder(const Parser& parser)
        : IsValid(parser.FastCheck())
        , Header(parser.GetHeader())
        , Stream({Header.BitStream, Header.SizeOfPacked - sizeof(Header.DstAddress)})
      {
        if (IsValid && !Stream.Eof())
        {
          IsValid = DecodeData();
        }
      }

      Binary::Container::Ptr GetResult()
      {
        return IsValid ? Decoded.CaptureResult() : Binary::Container::Ptr();
      }

    private:
      bool DecodeData()
      {
        Decoded = Binary::DataBuilder(MAX_DECODED_SIZE);

        while (!Stream.Eof() && Decoded.Size() < MAX_DECODED_SIZE)
        {
          //%0 - put byte
          if (!Stream.GetBit())
          {
            Decoded.AddByte(Stream.GetByte());
            continue;
          }
          uint_t len = Stream.GetLen();
          len += 2;
          if (2 == len)
          {
            const uint_t disp = Stream.GetByte() + 1;
            if (!CopyFromBack(disp, Decoded, 2))
            {
              return false;
            }
            continue;
          }
          if (5 == len)
          {
            const uint_t data = Stream.GetByte();
            if (0xff == data)
            {
              break;
            }
            if (0xfe == data)
            {
              len = Stream.GetLEWord();
            }
            else
            {
              len = data + 17;
            }
          }
          else if (len > 5)
          {
            --len;
          }
          const uint_t disp = GetOffset();
          if (!CopyFromBack(disp, Decoded, len))
          {
            return false;
          }
        }
        // last bytes are always copied from exact address
        const auto* lastBytes =
            static_cast<const uint8_t*>(Header.BitStream) + Header.SizeOfPacked - sizeof(Header.DstAddress);
        Decoded.Add(Binary::View{lastBytes, LAST_BYTES_COUNT});
        return true;
      }

      uint_t GetOffset()
      {
        if (Stream.GetBit())
        {
          return Stream.GetByte() + 1;
        }
        const uint_t code1 = Stream.GetBits(3);
        if (code1 < 2)
        {
          return 256 * (code1 + 1) + Stream.GetByte() + 1;
        }
        const uint_t code2 = 2 * code1 + Stream.GetBit();
        if (code2 < 8)
        {
          return 256 * (code2 - 1) + Stream.GetByte() + 1;
        }
        const uint_t code3 = 2 * code2 + Stream.GetBit();
        if (code3 < 0x17)
        {
          return 256 * (code3 - 9) + Stream.GetByte() + 1;
        }
        const uint_t code4 = (2 * code3 + Stream.GetBit()) & 0x1f;
        if (code4 < 0x1f)
        {
          return 256 * code4 + Stream.GetByte() + 1;
        }
        const uint_t hidisp = Stream.GetByte();
        const uint_t lodisp = Stream.GetByte();
        return 256 * hidisp + lodisp + 1;
      }

    private:
      bool IsValid;
      const RawHeader& Header;
      Hrust1::Bitstream Stream;
      Binary::DataBuilder Decoded;
    };

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
        DataDecoder decoder(parser);
        return CreateContainer(decoder.GetResult(), parser.GetUsedSize());
      }

    private:
      const Binary::Format::Ptr Format = Binary::CreateFormat(DEPACKER_PATTERN, MIN_SIZE);
    };
  }  // namespace MSPack

  Decoder::Ptr CreateMSPackDecoder()
  {
    return MakePtr<MSPack::Decoder>();
  }
}  // namespace Formats::Packed
