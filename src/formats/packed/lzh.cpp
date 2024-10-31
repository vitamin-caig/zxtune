/**
 *
 * @file
 *
 * @brief  LZH packer support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/container.h"
#include "formats/packed/pack_utils.h"

#include "binary/format_factories.h"
#include "formats/packed.h"

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
    class Container
    {
    public:
      Container(const void* data, std::size_t size)
        : Data(static_cast<const uint8_t*>(data))
        , Size(size)
      {}

      bool FastCheck() const
      {
        if (Size < sizeof(typename Version::RawHeader))
        {
          return false;
        }
        const typename Version::RawHeader& header = GetHeader();
        const DataMovementChecker checker(header.PackedDataSrc, header.PackedDataDst, header.PackedDataSize,
                                          header.PackedDataCopyDirection);
        if (!checker.IsValid())
        {
          return false;
        }
        const uint_t usedSize = GetUsedSize();
        return usedSize <= Size;
      }

      uint_t GetUsedSize() const
      {
        const typename Version::RawHeader& header = GetHeader();
        return offsetof(typename Version::RawHeader, DepackerBody) + header.DepackerBodySize + header.PackedDataSize;
      }

      const uint8_t* GetPackedData() const
      {
        const typename Version::RawHeader& header = GetHeader();
        const std::size_t fixedSize = offsetof(typename Version::RawHeader, DepackerBody) + header.DepackerBodySize;
        return Data + fixedSize;
      }

      std::size_t GetPackedSize() const
      {
        const typename Version::RawHeader& header = GetHeader();
        return header.PackedDataSize;
      }

      const typename Version::RawHeader& GetHeader() const
      {
        assert(Size >= sizeof(typename Version::RawHeader));
        return *safe_ptr_cast<const typename Version::RawHeader*>(Data);
      }

    private:
      const uint8_t* const Data;
      const std::size_t Size;
    };

    template<class Version>
    class DataDecoder
    {
    public:
      explicit DataDecoder(const Container<Version>& container)
        : IsValid(container.FastCheck())
        , Header(container.GetHeader())
        , Stream(container.GetPackedData(), container.GetPackedSize())
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
  }  // namespace LZH

  template<class Version>
  class LZHDecoder : public Decoder
  {
  public:
    LZHDecoder()
      : Depacker(Binary::CreateFormat(Version::DEPACKER_PATTERN, Version::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return Version::DESCRIPTION;
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
      const typename LZH::Container<Version> container(rawData.Start(), rawData.Size());
      if (!container.FastCheck())
      {
        return {};
      }
      typename LZH::DataDecoder<Version> decoder(container);
      return CreateContainer(decoder.GetResult(), container.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateLZH1Decoder()
  {
    return MakePtr<LZHDecoder<LZH::Version1> >();
  }

  Decoder::Ptr CreateLZH2Decoder()
  {
    return MakePtr<LZHDecoder<LZH::Version2> >();
  }
}  // namespace Formats::Packed
