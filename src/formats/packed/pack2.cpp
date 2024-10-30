/**
 *
 * @file
 *
 * @brief  Pack2 packer support
 *
 * @author vitamin.caig@gmail.com
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
// std includes
#include <algorithm>
#include <iterator>

namespace Formats::Packed
{
  namespace Pack2
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    const auto DESCRIPTION = "Pack v2.x"sv;
    const auto DEPACKER_PATTERN =
        "21??"  // ld hl,xxxx end of packed
        "11??"  // ld de,xxxx end of unpacked
        "e5"    // push hl
        "01??"  // ld bc,xxxx first of unpacked
        "b7"    // or a
        "ed42"  // sbc hl,bc
        "e1"    // pop hl
        "d8"    // ret c
        "7e"    // ld a,(hl)
        "fe?"   // cp xx
        "2804"  // jr z,xx
        "eda8"  // ldd
        "18ee"  // jr
        "2b"    // dec hl
        "46"    // ld b,(hl)
        "cb78"  // bit 7,b
        "2810"  // jr z,xxx
        "cbb8"  // res 7,b
        "78"    // ld a,b
        "a7"    // and a
        "280c"  // jr z,xxx
        "fe?"   // cp xx
        "3e?"   // ld a,xx
        "3006"  // jr nc,xx
        "3e?"   // ld a,xx
        "1802"  // jr xx
        "2b"    // dec hl
        "7e"    // ld a,(hl)
        "12"    // ld (de),a
        "1b"    // dec de
        "10fc"  // djnz xx
        "2b"    // dec hl
        "18cf"  // jr xx
        ""sv;

    struct RawHeader
    {
      //+0
      char Padding1;
      //+1
      le_uint16_t EndOfPacked;
      //+3
      char Padding2;
      //+4
      le_uint16_t EndOfUnpacked;
      //+6
      char Padding3[2];
      //+8
      le_uint16_t FirstOfUnpacked;
      //+0xa
      char Padding4[7];
      //+0x11
      uint8_t Marker;
      //+0x12
      char Padding5[0x13];
      //+0x25
      uint8_t RleThreshold;
      //+0x26
      char Padding6;
      //+0x27
      uint8_t SecondRleByte;
      //+0x28
      char Padding7[3];
      //+0x2b
      uint8_t FirstRleByte;
      //+0x2c
      char Padding8[0xb];
      //+0x37

      uint8_t PackedData[1];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x38, "Invalid layout");

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
        if (header.EndOfPacked < header.FirstOfUnpacked)
        {
          return false;
        }
        if (header.EndOfUnpacked < header.EndOfPacked)
        {
          return false;
        }
        const std::size_t usedSize = GetUsedSize();
        return usedSize <= Size;
      }

      std::size_t GetUsedSize() const
      {
        const RawHeader& header = GetHeader();
        const std::size_t selfAddr = header.FirstOfUnpacked - (sizeof(header) - 1);
        return header.EndOfPacked - selfAddr + 1;
      }

      std::size_t GetPackedSize() const
      {
        const RawHeader& header = GetHeader();
        return header.EndOfPacked - header.FirstOfUnpacked;
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

    class ReverseByteStream
    {
    public:
      ReverseByteStream(const uint8_t* data, std::size_t size)
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

    private:
      const uint8_t* const Data;
      const uint8_t* Pos;
    };

    class DataDecoder
    {
    public:
      explicit DataDecoder(const Container& container)
        : IsValid(container.FastCheck())
        , Header(container.GetHeader())
        , Stream(Header.PackedData, container.GetPackedSize())
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
        // assume that first byte always exists due to header format
        while (!Stream.Eof() && Decoded.Size() < MAX_DECODED_SIZE)
        {
          const uint_t data = Stream.GetByte();
          if (data != Header.Marker)
          {
            Decoded.AddByte(data);
          }
          else
          {
            const uint_t token = Stream.GetByte();
            if (token & 0x80)
            {
              if (const std::size_t len = token & 0x7f)
              {
                const uint8_t filler = len < Header.RleThreshold ? Header.FirstRleByte : Header.SecondRleByte;
                Fill(Decoded, len, filler);
              }
              else
              {
                Fill(Decoded, 256, 0);
              }
            }
            else
            {
              const std::size_t len = token ? token : 256;
              const uint8_t filler = Stream.GetByte();
              Fill(Decoded, len, filler);
            }
          }
        }
        Decoded.Resize(Decoded.Size() - 1);
        Reverse(Decoded);
        return true;
      }

    private:
      bool IsValid;
      const RawHeader& Header;
      ReverseByteStream Stream;
      Binary::DataBuilder Decoded;
    };
  }  // namespace Pack2

  class Pack2Decoder : public Decoder
  {
  public:
    Pack2Decoder()
      : Depacker(Binary::CreateFormat(Pack2::DEPACKER_PATTERN, Pack2::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return Pack2::DESCRIPTION;
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
      const Pack2::Container container(rawData.Start(), rawData.Size());
      if (!container.FastCheck())
      {
        return {};
      }
      Pack2::DataDecoder decoder(container);
      return CreateContainer(decoder.GetResult(), container.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreatePack2Decoder()
  {
    return MakePtr<Pack2Decoder>();
  }
}  // namespace Formats::Packed
