/**
 *
 * @file
 *
 * @brief  Hrum packer support
 *
 * @author vitamin.caig@gmail.com
 *
 * @note   Based on XLook sources by HalfElf
 *
 **/

#include "formats/packed/container.h"
#include "formats/packed/hrust1_bitstream.h"
#include "formats/packed/pack_utils.h"

#include <binary/format_factories.h>
#include <formats/packed.h>
#include <math/numeric.h>

#include <byteorder.h>
#include <make_ptr.h>
#include <pointers.h>

#include <numeric>

namespace Formats::Packed
{
  namespace Hrum
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    const auto DESCRIPTION = "Hrum v3.x"sv;
    const auto DEPACKER_PATTERN =
        "?"       // di/nop
        "ed73??"  // ld (xxxx),sp
        "21??"    // ld hl,xxxx   start+0x1f
        "11??"    // ld de,xxxx   tmp buffer
        "017700"  // ld bc,0x0077 size of depacker
        "d5"      // push de
        "edb0"    // ldir
        "11??"    // ld de,xxxx   dst of depack (data = +0x12)
        "d9"      // exx
        "21??"    // ld hl,xxxx   last byte of src packed (data = +0x16)
        "11??"    // ld de,xxxx   last byte of dst packed (data = +0x19)
        "01??"    // ld bc,xxxx   size of packed          (data = +0x1c)
        "c9"      // ret
        "ed?"     // lddr/ldir
        "16?"     // ld d,xx
        "31??"    // ld sp,xxxx   ;start of moved packed (data = +0x24)
        "c1"      // pop bc
        ""sv;

    struct RawHeader
    {
      //+0
      uint8_t Padding1[0x16];
      //+0x16
      le_uint16_t PackedSource;
      //+0x18
      uint8_t Padding2;
      //+0x19
      le_uint16_t PackedTarget;
      //+0x1b
      uint8_t Padding3;
      //+0x1c
      le_uint16_t SizeOfPacked;
      //+0x1e
      uint8_t Padding4[2];
      //+0x20
      uint8_t PackedDataCopyDirection;
      //+0x21
      uint8_t Padding5[3];
      //+0x24
      le_uint16_t FirstOfPacked;
      //+0x26
      uint8_t Padding6[0x6b];
      //+0x91
      uint8_t LastBytes[5];
      //+0x96 taken from stack to initialize variables, always 0x1010
      // packed data starts from here
      uint8_t Padding7[2];
      //+0x98
      uint8_t BitStream[2];
      //+0x9a
      uint8_t ByteStream[1];
      //+0x9b
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x9b, "Invalid layout");

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
        if (Size < sizeof(RawHeader))
        {
          return false;
        }
        const RawHeader& header = GetHeader();
        const DataMovementChecker checker(header.PackedSource, header.PackedTarget, header.SizeOfPacked,
                                          header.PackedDataCopyDirection);
        if (!checker.IsValid())
        {
          return false;
        }
        if (checker.FirstOfMovedData() != header.FirstOfPacked)
        {
          return false;
        }
        return GetUsedSize() <= Size;
      }

      uint_t GetUsedSize() const
      {
        const RawHeader& header = GetHeader();
        return sizeof(header) - (sizeof(header.Padding7) + sizeof(header.BitStream) + sizeof(header.ByteStream))
               + header.SizeOfPacked;
      }

      std::size_t GetUsedSizeWithPadding() const
      {
        const std::size_t usefulSize = GetUsedSize();
        const auto sizeOnDisk = Math::Align<std::size_t>(usefulSize, 256);
        const std::size_t resultSize = std::min(sizeOnDisk, Size);
        const std::size_t paddingSize = resultSize - usefulSize;
        const std::size_t MIN_SIGNATURE_MATCH = 9;
        if (paddingSize < MIN_SIGNATURE_MATCH)
        {
          return usefulSize;
        }
        // max padding size is 255 bytes
        // text is 78 bytes
        static const uint8_t HRUM3_5_PADDING[] = {
            'H',  'R',  'U',  'M',  ' ',  'v',  '3',  '.',  '5',  ' ',  'b',  'y',  ' ',  'D',  'm',  'i',  't',
            'r',  'y',  ' ',  'P',  'y',  'a',  'n',  'k',  'o',  'v',  0x80, 'T',  'e',  'l',  '.',  '(',  '3',
            '8',  '8',  '2',  '2',  ')',  '-',  '4',  '4',  '2',  '1',  '-',  '1',  '.',  'B',  'y',  'e',  '!',
            '!',  '!',  0x80, ' ',  'G',  'o',  'r',  'n',  'o',  '-',  'A',  'l',  't',  'a',  'y',  's',  'k',
            ',',  ' ',  '0',  '9',  '.',  '0',  '1',  '.',  '9',  '7',  0x81, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
            0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
        };
        static_assert(sizeof(HRUM3_5_PADDING) == 255, "Invalid layout");
        const uint8_t* const paddingStart = Data + usefulSize;
        const uint8_t* const paddingEnd = Data + resultSize;
        if (const std::size_t pad = MatchedSize(paddingStart, paddingEnd, HRUM3_5_PADDING, std::end(HRUM3_5_PADDING)))
        {
          if (pad >= MIN_SIGNATURE_MATCH)
          {
            return usefulSize + pad;
          }
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

    class Bitstream : public Hrust1Bitstream
    {
    public:
      Bitstream(const uint8_t* data, std::size_t size)
        : Hrust1Bitstream(data, size)
      {}

      uint_t GetDist()
      {
        if (GetBit())
        {
          const uint_t hiBits = 0xf0 + GetBits(4);
          return 256 * hiBits + GetByte();
        }
        else
        {
          return 0xff00 + GetByte();
        }
      }
    };

    class DataDecoder
    {
    public:
      explicit DataDecoder(const Container& container)
        : IsValid(container.FastCheck())
        , Header(container.GetHeader())
        , Stream(Header.BitStream, Header.SizeOfPacked - sizeof(Header.Padding7))
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
        // The main concern is to decode data as much as possible, skipping defenitely invalid structure
        Decoded = Binary::DataBuilder(2 * Header.SizeOfPacked);
        // put first byte
        Decoded.AddByte(Stream.GetByte());
        // assume that first byte always exists due to header format
        while (!Stream.Eof() && Decoded.Size() < MAX_DECODED_SIZE)
        {
          if (Stream.GetBit())
          {
            Decoded.AddByte(Stream.GetByte());
            continue;
          }
          uint_t len = 1 + Stream.GetLen();
          uint_t offset = 0;
          if (4 == len)
          {
            len = Stream.GetByte();
            if (!len)
            {
              // eof
              break;
            }
            offset = Stream.GetDist();
          }
          else
          {
            if (len > 4)
            {
              --len;
            }
            offset = DecodeOffsetByLen(len);
          }
          if (!CopyFromBack(-static_cast<int16_t>(offset), Decoded, len))
          {
            return false;
          }
        }
        // put remaining bytes
        Decoded.Add(Header.LastBytes);
        return true;
      }

    private:
      uint_t DecodeOffsetByLen(uint_t len)
      {
        if (1 == len)
        {
          return 0xfff8 + Stream.GetBits(3);
        }
        else if (2 == len)
        {
          return 0xff00 + Stream.GetByte();
        }
        else
        {
          return Stream.GetDist();
        }
      }

    private:
      bool IsValid;
      const RawHeader& Header;
      Bitstream Stream;
      Binary::DataBuilder Decoded;
    };
  }  // namespace Hrum

  class HrumDecoder : public Decoder
  {
  public:
    HrumDecoder()
      : Depacker(Binary::CreateFormat(Hrum::DEPACKER_PATTERN, Hrum::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return Hrum::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Depacker;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Depacker->Match(rawData))
      {
        return {};
      }
      const Hrum::Container container(rawData.Start(), rawData.Size());
      if (!container.FastCheck())
      {
        return {};
      }
      Hrum::DataDecoder decoder(container);
      return CreateContainer(decoder.GetResult(), container.GetUsedSizeWithPadding());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateHrumDecoder()
  {
    return MakePtr<HrumDecoder>();
  }
}  // namespace Formats::Packed
