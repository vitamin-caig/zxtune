/**
 *
 * @file
 *
 * @brief  ESVCruncher packer support
 *
 * @author vitamin.caig@gmail.com
 *
 * @note   Based on XLook sources by HalfElf
 *
 **/

// local includes
#include "formats/packed/container.h"
#include "formats/packed/pack_utils.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
#include <math/numeric.h>

namespace Formats::Packed
{
  namespace ESVCruncher
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    const auto DESCRIPTION = "ESV Cruncher"sv;
    const auto DEPACKER_PATTERN =
        //$=6978
        // depack to 9900/61a8
        "?"       // di/nop
        "21??"    // ld hl,xxxx                                        698f/698f
        "11??"    // ld de,xxxx                                        6000/5b00
        "01a300"  // ld bc,0x00a3 rest depacker size
        "d5"      // push de
        "edb0"    // ldir
        "21??"    // ld hl,xxxx last/first of packed (data = +#0e)     89b6/6a32
        "11??"    // ld de,xxxx last/first dst of packed (data = +#11) b884/61a7
        "01??"    // ld bc,xxxx size of packed  (data = +#14)          1f85/0815
        "c9"      // ret
        "ed?"     // lddr/lddr           +0x17                         b8/b0
        "21??"    // ld hl,xxxx last of packed (data = +#1a)           b884/69bb
        "010801"  // ld bc,xxxx                                        0108
        "d9"      // exx
        "e5"      // push hl
        "11??"    // ld de,xxxx last of depacked (data = +#22)         f929/72bb
        "210100"  // ld hl,xxxx                                        0001
                  /*
                  "cd??"    // call getbit                                         6099
                  "44"      // ld b,h
                  "3025"    // jr nc,xxxx
                  "45"      // ld b,l
                  "2c"      // inc l
                  "cd??"    // call getbit                                         6099
                  "382f"    // jr c,xx
                  "04"      // inc b
                  "2e04"    // ld l,xx
                  "cd??"    // call getbit                                         6099
                  "3827"    // jr c,xx
                  "cd??"    // call getbit                                         6099
                  "301e"    // jr nc,xxxx
                  "45"      // ld b,l
                  "cd??"    // call getbits                                         608c
                  "c608"    // ld a,xx
                  "6f"      // ld l,a
                  "1820"    // jr xxxx
                  "0605"    // ld b,xx
                  "cd??"    // call getbits                                         608c
                  "c60a"    // ld a,xx
                  "6f"      // ld l,a
                  "4d"      // ld c,l
                  "d9"      // exx
                  "e5"      // push hl
                  "d9"      // exx
                  "e1"      // pop hl
                  "edb8"    // lddr
                  "e5"      // push hl
                  "d9"      // exx
                  "e1"      // pop hl
                  "d9"      // exx
                  "1836"    // jr xxxx
                  "2e18"    // ld l,xx
                  "0608"    // ld b,xx
                  "cd??"    // call getbits                                         608c
                  "09"      // add hl,bc
                  "3c"      // inc a
                  "28f7"    // jr z,xx
                  "e5"      // push hl
                  "cd??"    // call getbit                                         6099
                  "212100"  // ld hl,xxxx                                        0021
                  "380f"    // jr c,xx
                  "0609"    // ld b,xx
                  "19"      // add hl,de
                  "cd??"    // call xxxx                                         6099
                  "3013"    // jr nc,xxxx
                  "0605"    // ld b,xx
                  "6b"      // ld l,e
                  "62"      // ld h,d
                  "23"      // inc hl
                  "180c"    // jr xxxx
                  "2602"    // ld h,xx
                  "c1"      // pop bc
                  "79"      // ld a,c
                  "94"      // sub h
                  "b0"      // or b
                  "28c0"    // jr z,xx
                  "c5"      // push bc
                  "06?"     // ld b,xx     ; window size (data = +0x8c)
                  "19"      // add hl,de
                  "cd??"    // call xxxx                                         608c
                  "09"      // add hl,bc
                  "c1"      // pop bc
                  "edb8"    // lddr
                  "21??"    // ld hl,xxxx  ; start of packed-1 (data = +           98ff/61a7
                  "ed52"    // sbc hl,de
                  "da??"    // jp c,xxxx
                  "e1"      // pop hl
                  "d9"      // exx
                  "?"       // di/ei/nop
                  "c3??"    // jp xxxx                                          0052/61a8
                  //getbits
                  "af"      // xor a
                  "4f"      // ld c,a
                  "cd??"    // call xxxx                                         6099
                  "cb11"    // rl c
                  "17"      // lra
                  "10f8"    // djnz xx
                  "47"      // ld b,a
                  "79"      // ld a,c
                  "c9"      // ret
                  //getbit
                  "d9"      // exx
                  "1003"    // djnz xx
                  "41"      // ld b,c
                  "5e"      // ld e,(hl)
                  "2b"      // dec hl
                  "cb13"    // rl e
                  "d9"      // exx
                  "c9"      // ret
                  */
        ""sv;

    struct RawHeader
    {
      //+0x00
      uint8_t Padding1[0x0e];
      //+0x0e
      le_uint16_t PackedSource;
      //+0x10
      uint8_t Padding2;
      //+0x11
      le_uint16_t PackedTarget;
      //+0x13
      uint8_t Padding3;
      //+0x14
      le_uint16_t SizeOfPacked;
      //+0x16
      uint8_t Padding4[2];
      //+0x18
      uint8_t PackedDataCopyDirection;
      //+0x19
      uint8_t Padding5;
      //+0x1a
      le_uint16_t LastOfPacked;
      //+0x1c
      uint8_t Padding6[0x6];
      //+0x22
      le_uint16_t LastOfDepacked;
      //+0x24
      uint8_t Padding7[0x68];
      //+0x8c
      uint8_t WindowSize;
      //+0x8d
      uint8_t Padding8[9];
      //+0x96
      le_uint16_t DepackedLimit;  // depacked - 1
      //+0x98
      uint8_t Padding9[0x22];
      //+0xba
      uint8_t Data[1];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0xba + 1, "Invalid layout");

    const std::size_t MIN_SIZE = sizeof(RawHeader);

    // dsq bitstream decoder
    class Bitstream
    {
    public:
      Bitstream(const uint8_t* data, std::size_t size)
        : Data(data)
        , Pos(Data + size)
      {}

      bool Eof() const
      {
        return Pos < Data;
      }

      uint8_t GetByte()
      {
        return Eof() ? 0 : *--Pos;
      }

      uint_t GetBit()
      {
        if (!(Mask >>= 1))
        {
          Bits = GetByte();
          Mask = 0x80;
        }
        return Bits & Mask ? 1 : 0;
      }

      uint_t GetBits(uint_t count)
      {
        uint_t result = 0;
        while (count--)
        {
          result = 2 * result | GetBit();
        }
        return result;
      }

    private:
      const uint8_t* const Data;
      const uint8_t* Pos;
      uint_t Bits = 0;
      uint_t Mask = 0;
    };

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
        if (!Math::InRange<uint_t>(header.WindowSize, 0x0b, 0x0f))
        {
          return false;
        }
        if (header.LastOfDepacked <= ((header.DepackedLimit + 1) & 0xffff))
        {
          return false;
        }
        const DataMovementChecker checker(header.PackedSource, header.PackedTarget, header.SizeOfPacked,
                                          header.PackedDataCopyDirection);
        if (!checker.IsValid())
        {
          return false;
        }
        if (checker.LastOfMovedData() != header.LastOfPacked)
        {
          return false;
        }
        const uint_t usedSize = GetUsedSize();
        return Size >= usedSize;
      }

      uint_t GetUsedSize() const
      {
        const RawHeader& header = GetHeader();
        return sizeof(header) + header.SizeOfPacked - sizeof(header.Data);
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
        , Stream(Header.Data, Header.SizeOfPacked)
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
        const uint_t unpackedSize = 1 + Header.LastOfDepacked - ((Header.DepackedLimit + 1) & 0xffff);
        Decoded = Binary::DataBuilder(unpackedSize);
        while (!Stream.Eof() && Decoded.Size() < unpackedSize)
        {
          if (!Stream.GetBit())
          {
            Decoded.AddByte(Stream.GetByte());
          }
          else if (!DecodeCmd())
          {
            return false;
          }
        }
        Reverse(Decoded);
        return true;
      }

      bool DecodeCmd()
      {
        const uint_t len = GetLength();
        if (const uint_t off = GetOffset(len))
        {
          return CopyFromBack(off, Decoded, len);
        }
        return true;
      }

      uint_t GetLength()
      {
        //%1
        if (Stream.GetBit())
        {
          //%11
          return 2 + Stream.GetBit();
        }
        //%10
        if (Stream.GetBit())
        {
          //%101
          return 4 + Stream.GetBits(2);
        }
        //%100
        if (Stream.GetBit())
        {
          //%1001
          return 8 + Stream.GetBits(4);
        }
        //%1000
        uint_t res = 0x18;
        for (uint_t add = 0xff; add == 0xff;)
        {
          add = Stream.GetBits(8);
          res += add;
        }
        return res;
      }

      uint_t GetOffset(uint_t len)
      {
        if (Stream.GetBit())
        {
          //%1
          if (2 != len)
          {
            return 0x221 + Stream.GetBits(Header.WindowSize);
          }
          const uint_t size = 0x0a + Stream.GetBits(5);
          Generate(Decoded, size, [stream = &Stream] { return stream->GetByte(); });
          return 0;
        }
        //%0
        if (Stream.GetBit())
        {
          //%01
          return 1 + Stream.GetBits(5);
        }
        //%00
        return 0x21 + Stream.GetBits(9);
      }

    private:
      bool IsValid;
      const RawHeader& Header;
      Bitstream Stream;
      Binary::DataBuilder Decoded;
    };
  }  // namespace ESVCruncher

  class ESVCruncherDecoder : public Decoder
  {
  public:
    ESVCruncherDecoder()
      : Depacker(Binary::CreateFormat(ESVCruncher::DEPACKER_PATTERN, ESVCruncher::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return ESVCruncher::DESCRIPTION;
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
      const ESVCruncher::Container container(rawData.Start(), rawData.Size());
      if (!container.FastCheck())
      {
        return {};
      }
      ESVCruncher::DataDecoder decoder(container);
      return CreateContainer(decoder.GetResult(), container.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateESVCruncherDecoder()
  {
    return MakePtr<ESVCruncherDecoder>();
  }
}  // namespace Formats::Packed
