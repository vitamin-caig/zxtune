/**
 *
 * @file
 *
 * @brief  LZH packer support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/common/container.h"
#include "formats/packed/common/utils.h"
#include "formats/packed/zx/utils.h"

#include "binary/format_factories.h"
#include "formats/packed/decoder.h"

#include "byteorder.h"
#include "make_ptr.h"
#include "pointers.h"
#include "string_view.h"

#include <algorithm>
#include <iterator>

namespace Formats::Packed
{
  namespace LZH
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    struct Version1
    {
      static const StringView DESCRIPTION;
      static const StringView DEPACKER_PATTERN;

      struct RawHeader
      {
        //+0
        char Padding1[2];
        //+2
        le_uint16_t DepackerBodySrc;
        //+4
        char Padding2;
        //+5
        le_uint16_t DepackerBodyDst;
        //+7
        char Padding3;
        //+8
        le_uint16_t DepackerBodySize;
        //+0xa
        char Padding4[4];
        //+0xe
        le_uint16_t PackedDataSrc;
        //+0x10
        char Padding5;
        //+0x11
        le_uint16_t PackedDataDst;
        //+0x13
        char Padding6;
        //+0x14
        le_uint16_t PackedDataSize;
        //+0x16
        char Padding7;
        //+0x17
        char DepackerBody[1];
        //+0x18
        uint8_t PackedDataCopyDirection;
        //+0x19
        char Padding8[2];
        //+0x1b
        le_uint16_t DepackingDst;
        //+0x1d
        char Padding9[0x3a];
        //+0x57
        uint8_t LastDepackedByte;
        //+0x58
      };

      static const std::size_t MIN_SIZE = sizeof(RawHeader);

      static std::size_t GetLZLen(uint_t data)
      {
        return (data & 15) + 3;
      }

      static std::size_t GetLZDistHi(uint_t data)
      {
        return (data & 0x70) << 4;
      }
    };

    struct Version2
    {
      static const StringView DESCRIPTION;
      static const StringView DEPACKER_PATTERN;

      struct RawHeader
      {
        //+0
        char Padding1[2];
        //+2
        le_uint16_t DepackerBodySrc;
        //+4
        char Padding2;
        //+5
        le_uint16_t DepackerBodyDst;
        //+7
        char Padding3;
        //+8
        le_uint16_t DepackerBodySize;
        //+0xa
        char Padding4[4];
        //+0xe
        le_uint16_t PackedDataSrc;
        //+0x10
        char Padding5;
        //+0x11
        le_uint16_t PackedDataDst;
        //+0x13
        char Padding6;
        //+0x14
        le_uint16_t PackedDataSize;
        //+0x16
        char Padding7;
        //+0x17
        char DepackerBody[1];
        //+0x18
        uint8_t PackedDataCopyDirection;
        //+0x19
        char Padding8[2];
        //+0x1b
        le_uint16_t DepackingDst;
        //+0x1d
        char Padding9[0x38];
        //+0x55
        uint8_t LastDepackedByte;
        //+0x56
      };

      static const std::size_t MIN_SIZE = sizeof(RawHeader);

      static std::size_t GetLZLen(uint_t data)
      {
        return ((data & 240) >> 4) - 5;
      }

      static std::size_t GetLZDistHi(uint_t data)
      {
        return (data & 15) << 8;
      }
    };

    const StringView Version1::DESCRIPTION = "LZH Compressor v1.4"sv;
    const StringView Version1::DEPACKER_PATTERN =
        "?"     // di/ei
        "21??"  // ld hl,xxxx depacker body src
        "11??"  // ld de,xxxx depacker body dst
        "01??"  // ld bc,xxxx depacker body size
        "d5"    // push de
        "edb0"  // ldir
        "21??"  // ld hl,xxxx packed src
        "11??"  // ld de,xxxx packed dst
        "01??"  // ld bc,xxxx packed size
        "c9"    // ret
        //+0x17
        "ed?"   // ldir/lddr
        "eb"    // ex de,hl
        "11??"  // ld de,depack dst
        "23"    // inc hl
        "7e"    // ld a,(hl)
        "cb7f"  // bit 7,a
        "28?"   // jr z,xx
        "e60f"  // and 0xf
        "c603"  // add a,3
        "4f"    // ld c,a
        "ed6f"  // rld
        "e607"  // and 7
        "47"    // ld b,a
        "23"    // inc hl
        "e5"    // push hl
        "7b"    // ld a,e
        "96"    // sub (hl)
        "6f"    // ld l,a
        ""sv;

    const StringView Version2::DESCRIPTION = "LZH Compressor v2.4"sv;
    const StringView Version2::DEPACKER_PATTERN =
        "?"     // di/ei
        "21??"  // ld hl,xxxx depacker body src
        "11??"  // ld de,xxxx depacker body dst
        "01??"  // ld bc,xxxx depacker body size
        "d5"    // push de
        "edb0"  // ldir
        "21??"  // ld hl,xxxx packed src
        "11??"  // ld de,xxxx packed dst
        "01??"  // ld bc,xxxx packed size
        "c9"    // ret
        //+0x17
        "ed?"   // ldir/lddr
        "eb"    // ex de,hl
        "11??"  // ld de,depack dst
        "23"    // inc hl
        "7e"    // ld a,(hl)
        "cb7f"  // bit 7,a
        "28?"   // jr z,xx
        "e60f"  // and 0xf
        "47"    // ld b,a
        "ed6f"  // rld
        "d605"  // sub 5
        "4f"    // ld c,a
        "23"    // inc hl
        "e5"    // push hl
        "7b"    // ld a,e
        "96"    // sub (hl)
        "6f"    // ld l,a
        ""sv;

    static_assert(sizeof(Version1::RawHeader) * alignof(Version1::RawHeader) == 0x58, "Invalid layout");
    static_assert(offsetof(Version1::RawHeader, DepackerBody) == 0x17, "Invalid layout");
    static_assert(sizeof(Version2::RawHeader) * alignof(Version2::RawHeader) == 0x56, "Invalid layout");
    static_assert(offsetof(Version2::RawHeader, DepackerBody) == 0x17, "Invalid layout");

    template<class Version>
    class Parser
    {
    public:
      explicit Parser(Binary::View data)
        : Data(data)
      {}

      bool FastCheck() const
      {
        if (Data.Size() < sizeof(typename Version::RawHeader))
        {
          return false;
        }
        const auto& header = GetHeader();
        if (!CheckDataMovement(header.PackedDataSrc, header.PackedDataDst, header.PackedDataSize,
                               header.PackedDataCopyDirection))
        {
          return false;
        }
        const uint_t usedSize = GetUsedSize();
        return usedSize <= Data.Size();
      }

      uint_t GetUsedSize() const
      {
        const auto& header = GetHeader();
        return offsetof(typename Version::RawHeader, DepackerBody) + header.DepackerBodySize + header.PackedDataSize;
      }

      auto GetPackedData() const
      {
        const auto& header = GetHeader();
        const std::size_t fixedSize = offsetof(typename Version::RawHeader, DepackerBody) + header.DepackerBodySize;
        return Data.SubView(fixedSize, header.PackedDataSize);
      }

      const auto& GetHeader() const
      {
        return *Data.As<typename Version::RawHeader>();
      }

    private:
      const Binary::View Data;
    };

    template<class Version>
    class DataDecoder
    {
    public:
      explicit DataDecoder(const Parser<Version>& parser)
        : IsValid(parser.FastCheck())
        , Header(parser.GetHeader())
        , Stream(parser.GetPackedData())
        , Decoded(MAX_DECODED_SIZE)
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
        // assume that first byte always exists due to header format
        while (!Stream.Eof() && Decoded.Size() < MAX_DECODED_SIZE)
        {
          const uint_t data = Stream.GetByte();
          if (!data)
          {
            break;
          }
          else if (0 != (data & 128))
          {
            const std::size_t len = Version::GetLZLen(data);
            const std::size_t offset = Version::GetLZDistHi(data) + Stream.GetByte() + 2;
            if (!CopyFromBack(offset, Decoded, len))
            {
              return false;
            }
          }
          else if (0 != (data & 64))
          {
            const std::size_t len = data - 0x3e + 1;
            Fill(Decoded, len, Stream.GetByte());
          }
          else
          {
            std::size_t len = data;
            for (; len && !Stream.Eof(); --len)
            {
              Decoded.AddByte(Stream.GetByte());
            }
            if (len)
            {
              return false;
            }
          }
        }
        Decoded.AddByte(Header.LastDepackedByte);
        return true;
      }

    private:
      bool IsValid;
      const typename Version::RawHeader& Header;
      ByteStream Stream;
      Binary::DataBuilder Decoded;
    };

    template<class Version>
    class Decoder : public Packed::Decoder
    {
    public:
      StringView GetDescription() const override
      {
        return Version::DESCRIPTION;
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
        const Parser<Version> parser(rawData);
        if (!parser.FastCheck())
        {
          return {};
        }
        DataDecoder<Version> decoder(parser);
        return CreateContainer(decoder.GetResult(), parser.GetUsedSize());
      }

    private:
      const Binary::Format::Ptr Format = Binary::CreateFormat(Version::DEPACKER_PATTERN, Version::MIN_SIZE);
    };
  }  // namespace LZH

  Decoder::Ptr CreateLZH1Decoder()
  {
    return MakePtr<LZH::Decoder<LZH::Version1> >();
  }

  Decoder::Ptr CreateLZH2Decoder()
  {
    return MakePtr<LZH::Decoder<LZH::Version2> >();
  }
}  // namespace Formats::Packed
