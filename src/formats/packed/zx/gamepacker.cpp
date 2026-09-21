/**
 *
 * @file
 *
 * @brief  GamePacker packer support
 *
 * @author vitamin.caig@gmail.com
 *
 * @note   Based on Pusher sources by Himik/ZxZ
 *
 **/

#include "formats/packed/common/container.h"
#include "formats/packed/common/utils.h"

#include "binary/format_factories.h"
#include "formats/packed/decoder.h"

#include "byteorder.h"
#include "make_ptr.h"
#include "pointers.h"
#include "string_view.h"

#include <cstring>

namespace Formats::Packed
{
  namespace GamePacker
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    struct Version1
    {
      static const StringView DESCRIPTION;
      static const std::size_t MIN_SIZE = 0x20;  // TODO
      static const StringView DEPACKER_PATTERN;

      struct RawHeader
      {
        //+0
        char Padding1;
        //+1
        le_uint16_t DepackerBodyAddr;
        //+3
        char Padding2[5];
        //+8
        le_uint16_t DepackerBodySize;
        //+a
        char Padding3[3];
        //+d
        le_uint16_t EndOfPackedSource;
        //+f
        char Padding4[4];
        //+13
        le_uint16_t PackedSize;
        //+15

        uint_t GetPackedDataOffset() const
        {
          const uint_t selfAddr = DepackerBodyAddr - 0x1d;
          const uint_t packedDataStart = EndOfPackedSource - PackedSize + 1;
          return packedDataStart - selfAddr;
        }

        uint_t GetPackedDataSize() const
        {
          return PackedSize;
        }
      };
    };

    struct Version2
    {
      static const StringView DESCRIPTION;
      static const std::size_t MIN_SIZE = 0x20;  // TODO
      static const StringView DEPACKER_PATTERN;

      struct RawHeader
      {
        //+0
        char Padding1;
        //+1
        le_uint16_t DepackerBodyAddr;
        //+3
        char Padding2[4];
        //+7
        le_uint16_t DepackerBodySize;
        //+9
        char Padding3[5];
        //+e
        le_uint16_t PackedSource;
        //+10

        uint_t GetPackedDataOffset() const
        {
          const uint_t selfAddr = DepackerBodyAddr - 0x0d;
          return PackedSource - selfAddr;
        }

        static uint_t GetPackedDataSize()
        {
          return 0;
        }
      };
    };

    const StringView Version1::DESCRIPTION = "GamePacker"sv;
    const StringView Version1::DEPACKER_PATTERN =
        "21??"    // ld hl,xxxx depacker body src
        "11??"    // ld de,xxxx depacker body dst
        "d5"      // push de
        "01??"    // ld bc,xxxx depacker body size
        "edb0"    // ldir
        "21??"    // ld hl,xxxx PackedSource
        "11ffff"  // ld de,ffff EndOfPacked
        "01??"    // ld bc,xxxx PackedSize
        "edb8"    // lddr
        "13"      // inc de
        "eb"      // ex de,hl
        "11??"    // ld de,depack target
        "c9"      // ret
        //+29 (0x1d) DepackerBody starts here
        "7c"  // ld a,h
        "b5"  // or l
        ""sv;

    const StringView Version2::DESCRIPTION = "GamePacker+"sv;
    const StringView Version2::DEPACKER_PATTERN =
        "21??"  // ld hl,xxxx depacker body src
        "11??"  // ld de,xxxx depacker body dst
        "01??"  // ld bc,xxxx depacker body size
        "d5"    // push de
        "edb0"  // ldir
        "c9"    // ret
        //+13 (0x0d)
        "21??"  // ld hl,xxxx PackedSource
        "11??"  // ld de,xxxx PackedTarget
        "7e"
        "cb7f"
        "20?"
        "e60f"
        "47"
        "ed6f"
        "23"
        "c603"
        "4f"
        "7b"
        "96"
        // 23 e5 6f 7a 98 67 0600 edb0 e1 18e3 e67f ca7181 23 cb77 2007 4f 0600 edb0 18d2 e6 3f c603 48 7e 23 12
        // 1310fc18c5
        ""sv;

    static_assert(sizeof(Version1::RawHeader) * alignof(Version1::RawHeader) == 0x15, "Invalid layout");
    static_assert(sizeof(Version2::RawHeader) * alignof(Version2::RawHeader) == 0x10, "Invalid layout");

    template<class Version>
    class Parser
    {
    public:
      explicit Parser(Binary::View data)
        : Data(data)
      {}

      bool Check() const
      {
        if (Data.Size() <= sizeof(typename Version::RawHeader))
        {
          return false;
        }
        const auto& header = GetHeader();
        return header.GetPackedDataOffset() + header.GetPackedDataSize() <= Data.Size();
      }

      Binary::View GetPackedData() const
      {
        const auto& header = GetHeader();
        if (const uint_t packed = header.GetPackedDataSize())
        {
          return Data.SubView(header.GetPackedDataOffset(), packed);
        }
        return Data.SubView(header.GetPackedDataOffset());
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
        : IsValid(parser.Check())
        , Header(parser.GetHeader())
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

      std::size_t GetUsedSize() const
      {
        return IsValid ? Header.GetPackedDataOffset() + Stream.GetProcessedBytes() : 0;
      }

    private:
      bool DecodeData()
      {
        Decoded = Binary::DataBuilder(Header.GetPackedDataSize() * 2);

        while (!Stream.Eof() && Decoded.Size() < MAX_DECODED_SIZE)
        {
          const uint8_t byt = Stream.GetByte();
          if (0 == (byt & 128))
          {
            const uint_t offset = 256 * (byt & 0x0f) + Stream.GetByte();
            const uint_t count = (byt >> 4) + 3;
            if (!CopyFromBack(offset, Decoded, count))
            {
              return false;
            }
          }
          else if (0 == (byt & 64))
          {
            uint_t count = byt & 63;
            if (!count)
            {
              return true;
            }
            for (; count && !Stream.Eof(); --count)
            {
              Decoded.AddByte(Stream.GetByte());
            }
            if (count)
            {
              return false;
            }
          }
          else
          {
            const uint_t data = Stream.GetByte();
            const uint_t len = (byt & 63) + 3;
            Fill(Decoded, len, data);
          }
        }
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
        if (!parser.Check())
        {
          return {};
        }
        DataDecoder<Version> decoder(parser);
        return CreateContainer(decoder.GetResult(), decoder.GetUsedSize());
      }

    private:
      const Binary::Format::Ptr Format = Binary::CreateFormat(Version::DEPACKER_PATTERN, Version::MIN_SIZE);
    };
  }  // namespace GamePacker

  Decoder::Ptr CreateGamePackerDecoder()
  {
    return MakePtr<GamePacker::Decoder<GamePacker::Version1> >();
  }

  Decoder::Ptr CreateGamePackerPlusDecoder()
  {
    return MakePtr<GamePacker::Decoder<GamePacker::Version2> >();
  }
}  // namespace Formats::Packed
