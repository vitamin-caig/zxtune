/**
 *
 * @file
 *
 * @brief  Hrust v1.x packer support
 *
 * @author vitamin.caig@gmail.com
 *
 * @note   Based on XLook sources by HalfElf
 *
 **/

// local includes
#include "formats/packed/container.h"
#include "formats/packed/hrust1_bitstream.h"
#include "formats/packed/pack_utils.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
#include <math/numeric.h>
// std includes
#include <numeric>

namespace Formats::Packed
{
  namespace Hrust1
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    const Char DESCRIPTION[] = "Hrust v1.x";
    const auto FORMAT = "'H'R"_sv;

    struct RawHeader
    {
      uint8_t ID[2];  //'HR'
      le_uint16_t DataSize;
      le_uint16_t PackedSize;
      uint8_t LastBytes[6];
      uint8_t BitStream[2];
      uint8_t ByteStream[1];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 15, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(RawHeader);

    class Container
    {
    public:
      Container(const void* data, std::size_t size)
        : Data(static_cast<const uint8_t*>(data))
        , Size(size)
      {}

      bool FastCheck() const
      {
        if (Size <= sizeof(RawHeader))
        {
          return false;
        }
        const RawHeader& header = GetHeader();
        const std::size_t usedSize = GetUsedSize();
        return Math::InRange(usedSize, sizeof(header), Size);
      }

      uint_t GetUsedSize() const
      {
        const RawHeader& header = GetHeader();
        return header.PackedSize;
      }

      std::size_t GetUsedSizeWithPadding() const
      {
        const std::size_t usefulSize = GetUsedSize();
        const std::size_t sizeOnDisk = Math::Align<std::size_t>(usefulSize, 256);
        const std::size_t resultSize = std::min(sizeOnDisk, Size);
        const std::size_t paddingSize = resultSize - usefulSize;
        const std::size_t TRDOS_ENTRY_SIZE = 16;
        const std::size_t TRDOS_SHORT_ENTRY_SIZE = 14;
        const std::size_t MIN_SIGNATURE_MATCH = 6;
        const std::size_t EXACT_SIGNATURE_MATCH = 17;  // to capture version
        if (paddingSize < TRDOS_ENTRY_SIZE + MIN_SIGNATURE_MATCH)
        {
          return usefulSize;
        }
        // max padding size is 255-14=241 bytes
        // text is 65 bytes
        static const uint8_t HRUST1_0_PADDING[] = {
            'h', 'r', 'u', 's', 't', '-', 'p', 'a', 'c', 'k', 'e', 'r', ' ', 'v', '1', '.', '0', ' ', 'b', 'y', ' ',
            'D', 'i', 'm', 'a', ' ', 'P', 'y', 'a', 'n', 'k', 'o', 'v', ',', 'G', '-', 'A', 'l', 't', 'a', 'i', 's',
            'k', ',', '1', '2', '.', '9', '7', ' ', 't', 'e', 'l', '(', '3', '8', '8', '2', '2', ')', '4', '4', '2',
            '1', '1', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        };
        // max padding size is 255-16=239 bytes
        // text is 48 bytes
        // Hrust1.1 really stores redunand sector if packed data is aligned by sector. Do not pay attention on that case
        static const uint8_t HRUST1_1_PADDING[] = {
            'h', 'r', 'u', 's', 't', '-', 'p', 'a', 'c', 'k', 'e', 'r', ' ', 'v', '1', '.', '1', ' ', 'b', 'y',
            ' ', 'D', 'i', 'm', 'a', ' ', 'P', 'y', 'a', 'n', 'k', 'o', 'v', ',', 'G', 'o', 'r', 'n', 'o', '-',
            'A', 'l', 't', 'a', 'i', 's', 'k', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
        static const uint8_t HRUST1_2_PADDING[] = {
            'h', 'r', 'u', 's', 't', '-', 'p', 'a', 'c', 'k', 'e', 'r', ' ', 'v', '1', '.', '2', ' ', 'b', 'y',
            ' ', 'D', 'i', 'm', 'a', ' ', 'P', 'y', 'a', 'n', 'k', 'o', 'v', ',', 'G', 'o', 'r', 'n', 'o', '-',
            'A', 'l', 't', 'a', 'i', 's', 'k', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
        static const uint8_t HRUST1_3_PADDING[] = {
            'h', 'r', 'u', 's', 't', '-', 'p', 'a', 'c', 'k', 'e', 'r', ' ', 'v', '1', '.', '3', ' ', 'b', 'y',
            ' ', 'D', 'i', 'm', 'a', ' ', 'P', 'y', 'a', 'n', 'k', 'o', 'v', ',', 'G', 'o', 'r', 'n', 'o', '-',
            'A', 'l', 't', 'a', 'i', 's', 'k', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
        static_assert(sizeof(HRUST1_0_PADDING) == 255 - TRDOS_SHORT_ENTRY_SIZE, "Invalid layout");
        static_assert(sizeof(HRUST1_1_PADDING) == 255 - TRDOS_ENTRY_SIZE, "Invalid layout");
        static_assert(sizeof(HRUST1_2_PADDING) == 255 - TRDOS_ENTRY_SIZE, "Invalid layout");
        static_assert(sizeof(HRUST1_3_PADDING) == 255 - TRDOS_ENTRY_SIZE, "Invalid layout");
        const uint8_t* const paddingStart = Data + usefulSize;
        const uint8_t* const paddingEnd = Data + resultSize;
        // special case due to distinct offset
        const std::size_t padv0 = MatchedSize(paddingStart + TRDOS_SHORT_ENTRY_SIZE, paddingEnd, HRUST1_0_PADDING,
                                              std::end(HRUST1_0_PADDING));
        if (padv0 >= MIN_SIGNATURE_MATCH)
        {
          // version 1.0 match
          return usefulSize + TRDOS_SHORT_ENTRY_SIZE + padv0;
        }
        const std::size_t padv1 =
            MatchedSize(paddingStart + TRDOS_ENTRY_SIZE, paddingEnd, HRUST1_1_PADDING, std::end(HRUST1_1_PADDING));
        if (padv1 >= EXACT_SIGNATURE_MATCH)
        {
          // version 1.1 match
          return usefulSize + TRDOS_ENTRY_SIZE + padv1;
        }
        const std::size_t padv2 =
            MatchedSize(paddingStart + TRDOS_ENTRY_SIZE, paddingEnd, HRUST1_2_PADDING, std::end(HRUST1_2_PADDING));
        if (padv2 >= EXACT_SIGNATURE_MATCH)
        {
          // version 1.2 match
          return usefulSize + TRDOS_ENTRY_SIZE + padv2;
        }
        const std::size_t padv3 =
            MatchedSize(paddingStart + TRDOS_ENTRY_SIZE, paddingEnd, HRUST1_3_PADDING, std::end(HRUST1_3_PADDING));
        if (padv3 >= EXACT_SIGNATURE_MATCH)
        {
          // version 1.3 match
          return usefulSize + TRDOS_ENTRY_SIZE + padv3;
        }
        const std::size_t pad = std::min(padv1, std::min(padv2, padv3));
        if (pad >= MIN_SIGNATURE_MATCH)
        {
          return usefulSize + TRDOS_ENTRY_SIZE + pad;
        }
        return usefulSize;
      }

      const RawHeader& GetHeader() const
      {
        assert(Size >= sizeof(RawHeader));
        return *safe_ptr_cast<const RawHeader*>(Data);
      }

    private:
      const uint8_t* const Data;
      const std::size_t Size;
    };

    class DataDecoder
    {
    public:
      explicit DataDecoder(const Container& container)
        : IsValid(container.FastCheck())
        , Header(container.GetHeader())
        , Stream(Header.BitStream, container.GetUsedSize() - 12)
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
        Decoded = Binary::DataBuilder(Header.DataSize);

        // put first byte
        Decoded.AddByte(Stream.GetByte());
        uint_t refBits = 2;
        while (!Stream.Eof() && Decoded.Size() < MAX_DECODED_SIZE)
        {
          //%1 - put byte
          if (Stream.GetBit())
          {
            Decoded.AddByte(Stream.GetByte());
            continue;
          }
          uint_t len = Stream.GetLen();

          //%0 00,-disp3 - copy byte with offset
          if (0 == len)
          {
            const int_t offset = static_cast<int16_t>(0xfff8 + Stream.GetBits(3));
            if (!CopyByteFromBack(offset))
            {
              return false;
            }
            continue;
          }
          //%0 01 - copy 2 bytes
          else if (1 == len)
          {
            const uint_t code = Stream.GetBits(2);
            int_t offset = 0;
            //%10: -dispH=0xff
            if (2 == code)
            {
              uint_t byte = Stream.GetByte();
              if (byte >= 0xe0)
              {
                byte <<= 1;
                ++byte;
                byte ^= 2;
                byte &= 0xff;

                if (byte == 0xff)  // inc refsize
                {
                  ++refBits;
                  continue;
                }
                offset = static_cast<int16_t>(0xff00 + byte - 0xf);
                if (!CopyBreaked(offset))
                {
                  return false;
                }
                continue;
              }
              offset = static_cast<int16_t>(0xff00 + byte);
            }
            //%00..01: -dispH=#fd..#fe,-dispL
            else if (0 == code || 1 == code)
            {
              offset = static_cast<int16_t>((code ? 0xfe00 : 0xfd00) + Stream.GetByte());
            }
            //%11,-disp5
            else if (3 == code)
            {
              offset = static_cast<int16_t>(0xffe0 + Stream.GetBits(5));
            }
            if (!CopyFromBack(-offset, Decoded, 2))
            {
              return false;
            }
            continue;
          }
          //%0 1100...
          else if (3 == len)
          {
            //%011001
            if (Stream.GetBit())
            {
              const int_t offset = static_cast<int16_t>(0xfff0 + Stream.GetBits(4));
              if (!CopyBreaked(offset))
              {
                return false;
              }
              continue;
            }
            //%0110001
            else if (Stream.GetBit())
            {
              const uint_t count = 2 * (6 + Stream.GetBits(4));
              for (uint_t bytes = 0; bytes < count; ++bytes)
              {
                Decoded.AddByte(Stream.GetByte());
              }
              continue;
            }
            else
            {
              len = Stream.GetBits(7);
              if (0xf == len)
              {
                break;  // EOF
              }
              else if (len < 0xf)
              {
                len = 256 * len + Stream.GetByte();
              }
            }
          }

          if (2 == len)
          {
            ++len;
          }
          const uint_t code = Stream.GetBits(2);
          int_t offset = 0;
          if (1 == code)
          {
            uint_t byte = Stream.GetByte();
            if (byte >= 0xe0)
            {
              if (len > 3)
              {
                return false;
              }
              byte <<= 1;
              ++byte;
              byte ^= 3;
              byte &= 0xff;

              offset = static_cast<int16_t>(0xff00 + byte - 0xf);
              if (!CopyBreaked(offset))
              {
                return false;
              }
              continue;
            }
            offset = static_cast<int16_t>(0xff00 + byte);
          }
          else if (0 == code)
          {
            offset = static_cast<int16_t>(0xfe00 + Stream.GetByte());
          }
          else if (2 == code)
          {
            offset = static_cast<int16_t>(0xffe0 + Stream.GetBits(5));
          }
          else if (3 == code)
          {
            static const uint_t Mask[] = {0, 0, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0};
            offset = 256 * (Mask[refBits] + Stream.GetBits(refBits));
            offset |= Stream.GetByte();
            offset = static_cast<int16_t>(offset & 0xffff);
          }
          if (!CopyFromBack(-offset, Decoded, len))
          {
            return false;
          }
        }
        // put remaining bytes
        Decoded.Add(Header.LastBytes);
        return true;
      }

      bool CopyByteFromBack(int_t offset)
      {
        assert(offset <= 0);
        const std::size_t size = Decoded.Size();
        if (uint_t(-offset) > size)
        {
          return false;  // invalid backreference
        }
        const auto val = Decoded.Get<uint8_t>(size + offset);
        Decoded.AddByte(val);
        return true;
      }

      bool CopyBreaked(int_t offset)
      {
        return CopyByteFromBack(offset) && (Decoded.AddByte(Stream.GetByte()), true) && CopyByteFromBack(offset);
      }

    private:
      bool IsValid;
      const RawHeader& Header;
      Hrust1Bitstream Stream;
      Binary::DataBuilder Decoded;
    };
  }  // namespace Hrust1

  class Hrust1Decoder : public Decoder
  {
  public:
    Hrust1Decoder()
      : Format(Binary::CreateFormat(Hrust1::FORMAT, Hrust1::MIN_SIZE))
    {}

    String GetDescription() const override
    {
      return Hrust1::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Format->Match(rawData))
      {
        return Container::Ptr();
      }
      const Hrust1::Container container(rawData.Start(), rawData.Size());
      if (!container.FastCheck())
      {
        return Container::Ptr();
      }
      Hrust1::DataDecoder decoder(container);
      return CreateContainer(decoder.GetResult(), container.GetUsedSizeWithPadding());
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateHrust1Decoder()
  {
    return MakePtr<Hrust1Decoder>();
  }
}  // namespace Formats::Packed
