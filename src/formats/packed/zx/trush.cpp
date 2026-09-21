/**
 *
 * @file
 *
 * @brief  Trush packer support
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

#include <cstring>

namespace Formats::Packed
{
  namespace Trush
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    const auto DESCRIPTION = "Trush Compressor v3.x"sv;
    // Head and tail are delimited by optional signature (some versions store additional code there)

    // At least two different prefixes
    // using ix/iy
    // Depacker beginning may be corrupted
    const auto DEPACKER_HEAD =
        "???"          // di/ei      | jp xxxx
                       // ld b,0x10
        "?"            // exx
        "?"            // push hl
        "???"          // ld hl,xxxx
        "???"          // ld de,xxxx
        "01??"         // ld bc,xxxx ;packed block size
        "d5"           // push de
        "ed(b0|b8)"    // ldir/lddr
        "eb"           // ex de,hl
        "23"           // inc hl
        "(dd|fd)21??"  // ld ix/y,xxxxx
        "(dd|fd)39"    // add ix/y,sp
        "f9"           // ld sp,hl
        "21??"         // ld hl,xxxx ;addr of body
        "11??"         // ld de,xxxx
        "01??"         // ld bc,xxxx ;size of body
        "d5"           // push de
        "c3??"         // jp xxxx
        ""sv;

    const std::size_t HEAD_SIZE = 0x27;

    // There's some screen compressor with the almost same depacker:
    /*
      +c0
         ld sp,ix
         ld hl,xxxx
         exx
         ei
         ret
    */
    const auto DEPACKER_BODY =
        "d9"    // exx
        "e1"    // pop hl
        "1806"  // jr xx
        "3b"    // dec sp
        "f1"    // pop af
        "d9"    // exx
        "12"    // ld (de),a
        "13"    // inc de
        "d9"    // exx
        "29"    // add hl,hl
        "1003"  // djnz xx
        "e1"    // pop hl
        "0610"  // ld b,xx
                /*
                //+0x10
                "?{176}"
                //+0xc0
                "?f9"   //ld sp,ix/y
                "1b"     //dec de
                "eb"     //ex de,hl
                "d1"     //pop de
                "01??"   //ld bc,xxxx
                "ed?"    //lddr
                //several variants
                "?"      //pop hl | pop hl  | jp xxxx | di
                "?"      //exx    | jp xxxx |         | jp xxxx
                "?"      //ei/nop |         |
                "?"      //ret    |         | nop
                */
        ""sv;

    const std::size_t BODY_SIZE = 0xce;

    struct RawHeader
    {
      //+0
      char Padding1[0x0c];
      //+c
      le_uint16_t SizeOfPacked;
      //+e
      char Padding2[0x13];
      //+21
      le_uint16_t DepackerBodySize;
    };

    const std::size_t MIN_DATA_SIZE = 2;
    const std::size_t MAX_HEAD_SIZE = HEAD_SIZE + 36;  // typical signature
    const std::size_t MIN_BODY_SIZE = BODY_SIZE + MIN_DATA_SIZE;

    class Parser
    {
    public:
      Parser(Binary::View data, std::size_t bodyOffset)
        : Data(data)
        , DepackerSize(bodyOffset + GetBodySize())
      {}

      bool FastCheck() const
      {
        if (Data.Size() <= DepackerSize + MIN_DATA_SIZE)
        {
          return false;
        }
        const std::size_t usedSize = GetUsedSize();
        return Math::InRange(usedSize, DepackerSize, Data.Size());
      }

      uint_t GetUsedSize() const
      {
        return DepackerSize + GetPackedSize();
      }

      auto GetPackedData() const
      {
        return Data.SubView(DepackerSize, GetPackedSize());
      }

    private:
      const RawHeader& GetHeader() const
      {
        return *Data.As<RawHeader>();
      }

      std::size_t GetBodySize() const
      {
        return GetHeader().DepackerBodySize;
      }

      std::size_t GetPackedSize() const
      {
        return GetHeader().SizeOfPacked;
      }

    private:
      const Binary::View Data;
      const std::size_t DepackerSize;
    };

    class DataDecoder
    {
    public:
      explicit DataDecoder(const Parser& parser)
        : IsValid(parser.FastCheck())
        , Stream(parser.GetPackedData())
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
        Decoded = Binary::DataBuilder(Stream.GetRestBytes() * 2);

        while (!Stream.Eof() && Decoded.Size() < MAX_DECODED_SIZE)
        {
          //%0 - put byte
          if (!Stream.GetBit())
          {
            Decoded.AddByte(Stream.GetByte());
            continue;
          }
          uint_t code = Stream.GetBits(2);
          if (code)
          {
            code = 2 * code + Stream.GetBit();
            if (code >= 6)
            {
              code = 2 * code + Stream.GetBit();
            }
          }
          if (2 == code)
          {
            const uint_t offset = Stream.GetByte() + 1;
            if (!CopyFromBack(offset, Decoded, 2))
            {
              return false;
            }
            continue;
          }
          uint_t len = code < 2 ? code + 3 : 0;
          if (3 == code)
          {
            const uint_t data = Stream.GetByte();
            if (0xff == data)
            {
              break;
            }
            else if (0xfe == data)
            {
              len = Stream.GetLEWord();
            }
            else
            {
              len = data + 10;
            }
          }
          else if (code > 3)
          {
            len = code;
            if (len >= 6)
            {
              len -= 6;
            }
          }
          const uint_t offset = GetOffset();
          if (!CopyFromBack(offset, Decoded, len))
          {
            return false;
          }
        }
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
        const uint_t lodisp = Stream.GetByte();
        return 256 * code4 + lodisp + 1;
      }

    private:
      bool IsValid;
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
        const Parser parser(rawData, DepackerBody->NextMatchOffset(rawData));
        if (!parser.FastCheck())
        {
          return {};
        }
        DataDecoder decoder(parser);
        return CreateContainer(decoder.GetResult(), parser.GetUsedSize());
      }

    private:
      const Binary::ScanningFormat::Ptr DepackerBody = Binary::CreateScanningFormat(DEPACKER_BODY, MIN_BODY_SIZE);
      const Binary::ScanningFormat::Ptr Format = Binary::CreateCompositeFormat(
          Binary::CreateScanningFormat(DEPACKER_HEAD), DepackerBody, HEAD_SIZE, MAX_HEAD_SIZE);
    };
  }  // namespace Trush

  Decoder::Ptr CreateTRUSHDecoder()
  {
    return MakePtr<Trush::Decoder>();
  }
}  // namespace Formats::Packed
