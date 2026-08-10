/**
 *
 * @file
 *
 * @brief  CompressorCode v4 & v4+ packer support
 *
 * @author vitamin.caig@gmail.com
 *
 * @note   Based on XLook sources by HalfElf
 *
 **/

#include "formats/packed/common/container.h"
#include "formats/packed/common/utils.h"

#include "binary/format_factories.h"
#include "formats/packed/decoder.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"
#include "string_view.h"

#include <algorithm>
#include <iterator>

namespace Formats::Packed
{
  namespace CompressorCode
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    struct Version4
    {
      static const StringView DESCRIPTION;
      static const std::size_t MIN_SIZE = 256;  // TODO
      static const StringView DEPACKER_PATTERN;

      struct RawHeader
      {
        //+0
        uint8_t Padding1[0x0e];
        //+0x0e
        le_uint16_t RestDepackerSize;
        //+0x10
        uint8_t Padding2[0x0a];
        //+0x1a
        le_uint16_t ChunksCount;
        //+0x1c
      };

      static bool FastCheck(const RawHeader& header, std::size_t size)
      {
        const uint_t depackerSize = 0x14 + header.RestDepackerSize;
        const uint_t chunksCount = header.ChunksCount;
        // at least one byte per chunk
        if (chunksCount > MAX_DECODED_SIZE || depackerSize + chunksCount > size)
        {
          return false;
        }
        const uint8_t RET_CODE = 0xc9;
        const auto* const depacker = safe_ptr_cast<const uint8_t*>(&header);
        return depacker[depackerSize - 1] == RET_CODE;
      }

      static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x1c, "Invalid layout");
    };

    struct Version4Plus
    {
      static const StringView DESCRIPTION;
      static const std::size_t MIN_SIZE = 256;  // TODO
      static const StringView DEPACKER_PATTERN;

      struct RawHeader
      {
        //+0
        uint8_t Padding1[0x0e];
        //+0x0e
        le_uint16_t RestDepackerSize;
        //+0x10
        uint8_t Padding2[0x0d];
        //+0x1d
        le_uint16_t PackedDataSize;
        //+0x1f
        uint8_t Padding3[0x15];
        //+0x34
        le_uint16_t ChunksCount;
        //+0x36
      };

      static bool FastCheck(const RawHeader& header, std::size_t size)
      {
        const uint_t depackerSize = 0x14 + header.RestDepackerSize;
        const uint_t chunksCount = header.ChunksCount;
        // at least one byte per chunk
        if (chunksCount > MAX_DECODED_SIZE || depackerSize > size || !header.PackedDataSize || depackerSize < 257)
        {
          return false;
        }
        const auto* const retPos = static_cast<const uint8_t*>(header.Padding1) + depackerSize - 256 - 1;
        return *retPos == 0xc9;
      }

      static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x36, "Invalid layout");
    };

    const StringView Version4::DESCRIPTION = "CompressorCode v.4 by ZYX"sv;
    const StringView Version4::DEPACKER_PATTERN =
        "cd5200"  // call 0x52
        "3b"      // dec sp
        "3b"      // dec sp
        "e1"      // pop hl
        "011100"  // ld bc,0x0011 ;0x07
        "09"      // add hl,bc
        "11??"    // ld de,xxxx ;depacker addr
        "01??"    // ld bc,xxxx ;depacker size +e
        "d5"      // push de
        "edb0"    // ldir
        "c9"      // ret
        // add hl,bc points here +14
        "fde5"  // push iy
        "11??"  // ld de,xxxx ;dst addr
        "01??"  // ld bc,xxxx ;packed size +1a
        "c5"    // push bc
        "01??"  // ld bc,xxxx
        "c5"    // push bc
        ""sv;

    const StringView Version4Plus::DESCRIPTION = "CompressorCode v.4+ by ZYX"sv;
    const StringView Version4Plus::DEPACKER_PATTERN =
        "cd5200"  // call 0x52
        "3b"      // dec sp
        "3b"      // dec sp
        "e1"      // pop hl
        "011100"  // ld bc,0x0011 ;0x07
        "09"      // add hl,bc
        "11??"    // ld de,xxxx ;depacker addr
        "01??"    // ld bc,xxxx ;depacker size +e
        "d5"      // push de
        "edb0"    // ldir
        "c9"      // ret
        // add hl,bc points here +14
        "fde5"  // push iy
        "e5"    // push hl
        "dde1"  // pop ix
        "11??"  // ld de,xxxx ;dst addr
        "01??"  // ld bc,xxxx ;huffman packed size +1d
        ""sv;

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
        return Version::FastCheck(header, Data.Size());
      }

      std::size_t GetAvailableData() const
      {
        return Data.Size();
      }

      const auto& GetHeader() const
      {
        return *Data.As<const typename Version::RawHeader>();
      }

    private:
      const Binary::View Data;
    };

    class StreamAdapter : public ByteStream
    {
    public:
      explicit StreamAdapter(Binary::View data)
        : ByteStream(data)
      {}

      uint8_t operator*()
      {
        Require(!Eof());
        return GetByte();
      }

      StreamAdapter& operator++(int)
      {
        return *this;
      }
    };

    class RawDataDecoder
    {
    public:
      RawDataDecoder(Binary::View data, uint_t chunksCount)
        : Stream(data)
        , ChunksCount(chunksCount)
        , Decoded(2 * ChunksCount)
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
        return Stream.GetProcessedBytes();
      }

    private:
      bool DecodeData()
      {
        try
        {
          uint_t chunksCount = ChunksCount;
          StreamAdapter& source(Stream);
          // assume that first byte always exists due to header format
          while (chunksCount-- && Decoded.Size() < MAX_DECODED_SIZE)
          {
            const uint8_t data = *source;
            const uint_t count = (data >> 5);
            if (24 == (data & 24))
            {
              const uint_t offset = 256 * (data & 7) + *source;
              if (!CopyFromBack(offset, Decoded, count + 3))
              {
                return false;
              }
              continue;
            }
            uint_t len = (data & 1) ? 256 * count + *source : count + 3;
            switch (data & 0x1f)
            {
            case 0:
              len -= 2;
              [[fallthrough]];
            case 1:
              while (len--)
              {
                Decoded.AddByte(*source);
              }
              break;
            case 2:
              --len;
              [[fallthrough]];
            case 3:
              Fill(Decoded, len, 0);
              break;
            case 4:
              --len;
              [[fallthrough]];
            case 5:
              Fill(Decoded, len, 0xff);
              break;
            case 6:
            case 7:
              Fill(Decoded, len, *source);
              break;
            case 8:
              ++len;
              [[fallthrough]];
            case 9:
              for (uint8_t data = *source, delta = *source; len; --len, data += delta)
              {
                Decoded.AddByte(data);
              }
              break;
            case 0xa:
              --len;
              [[fallthrough]];
            case 0xb:
              for (const uint8_t data1 = *source, data2 = *source; len; --len)
              {
                Decoded.AddByte(data1);
                Decoded.AddByte(data2);
              }
              break;
            case 0xc:
              --len;
              [[fallthrough]];
            case 0xd:
              for (const uint8_t data1 = *source, data2 = *source, data3 = *source; len; --len)
              {
                Decoded.AddByte(data1);
                Decoded.AddByte(data2);
                Decoded.AddByte(data3);
              }
              break;
            case 0xe:
            case 0xf:
              for (const uint8_t data = *source; len; --len)
              {
                Decoded.AddByte(data);
                Decoded.AddByte(*source);
              }
              break;
            case 0x10:
            case 0x11:
              for (const uint8_t data = *source; len; --len)
              {
                Decoded.AddByte(data);
                Decoded.AddByte(*source);
                Decoded.AddByte(*source);
              }
              break;
            case 0x12:
            case 0x13:
              for (const uint8_t base = *source; len; --len)
              {
                const uint8_t data = *source;
                Decoded.AddByte(static_cast<uint8_t>(base + (data >> 4)));
                Decoded.AddByte(static_cast<uint8_t>(base + (data & 0x0f)));
              }
              break;
            case 0x14:
            case 0x15:
            {
              const uint_t offset = *source;
              if (!CopyFromBack(offset, Decoded, len))
              {
                return false;
              }
            }
            break;
            case 0x16:
              ++len;
              [[fallthrough]];
            case 0x17:
            {
              const uint_t hiOff = *source;
              const uint_t loOff = *source;
              const uint_t offset = 256 * hiOff + loOff;
              if (!CopyFromBack(offset, Decoded, len))
              {
                return false;
              }
            }
            break;
            }
          }
          return true;
        }
        catch (const std::exception&)
        {
          return false;
        }
      }

    private:
      bool IsValid = true;
      StreamAdapter Stream;
      const uint_t ChunksCount;
      Binary::DataBuilder Decoded;
    };

    template<class Version>
    class DataDecoder;

    template<>
    class DataDecoder<Version4>
    {
    public:
      explicit DataDecoder(const Parser<Version4>& parser)
        : Header(parser.GetHeader())
        , DataOffset(0x14 + Header.RestDepackerSize)
        , Delegate(parser.FastCheck() ? new RawDataDecoder({static_cast<const uint8_t*>(Header.Padding1) + DataOffset,
                                                            parser.GetAvailableData() - DataOffset},
                                                           Header.ChunksCount)
                                      : nullptr)
      {}

      Binary::Container::Ptr GetResult()
      {
        return Delegate ? Delegate->GetResult() : Binary::Container::Ptr();
      }

      std::size_t GetUsedSize() const
      {
        return Delegate ? DataOffset + Delegate->GetUsedSize() : 0;
      }

    private:
      const Version4::RawHeader& Header;
      const std::size_t DataOffset;
      const std::unique_ptr<RawDataDecoder> Delegate;
    };

    class Bitstream
    {
    public:
      explicit Bitstream(Binary::View data)
        : Source(data)
      {}

      std::size_t GetUsedData() const
      {
        return Source.GetProcessedBytes();
      }

      uint_t GetBit()
      {
        if (!(Mask >>= 1))
        {
          Bits = *Source;
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

      uint_t GetIndex()
      {
        if (const uint_t len = GetBits(3))
        {
          return GetBits(len) | (1 << len);
        }
        else
        {
          return GetBit();
        }
      }

    private:
      StreamAdapter Source;
      uint_t Bits = 0;
      uint_t Mask = 0;
    };

    template<>
    class DataDecoder<Version4Plus>
    {
    public:
      explicit DataDecoder(const Parser<Version4Plus>& parser)
        : Header(parser.GetHeader())
        , DataOffset(0x14 + Header.RestDepackerSize)
      {
        if (parser.FastCheck() && DecodeHuffman(parser.GetAvailableData() - DataOffset))
        {
          Delegate.reset(
              new RawDataDecoder({&UnhuffmanData.Get<uint8_t>(0), UnhuffmanData.Size()}, Header.ChunksCount));
        }
      }

      Binary::Container::Ptr GetResult()
      {
        return Delegate ? Delegate->GetResult() : Binary::Container::Ptr();
      }

      std::size_t GetUsedSize() const
      {
        return Delegate ? DataOffset + DataSize : 0;
      }

    private:
      bool DecodeHuffman(std::size_t availableSize)
      {
        try
        {
          const auto* data = static_cast<const uint8_t*>(Header.Padding1) + DataOffset;
          Bitstream stream({data, availableSize});
          const auto* table = data - 256;
          for (uint_t packedBytes = Header.PackedDataSize; packedBytes; --packedBytes)
          {
            const uint_t idx = stream.GetIndex();
            UnhuffmanData.AddByte(table[idx]);
          }
          DataSize = stream.GetUsedData();
          return true;
        }
        catch (const std::exception&)
        {
          return false;
        }
      }

    private:
      const Version4Plus::RawHeader& Header;
      const uint_t DataOffset;
      std::size_t DataSize = 0;
      Binary::DataBuilder UnhuffmanData;
      std::unique_ptr<RawDataDecoder> Delegate;
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
        return CreateContainer(decoder.GetResult(), decoder.GetUsedSize());
      }

    private:
      const Binary::Format::Ptr Format = Binary::CreateFormat(Version::DEPACKER_PATTERN, Version::MIN_SIZE);
    };
  }  // namespace CompressorCode

  Decoder::Ptr CreateCompressorCode4Decoder()
  {
    return MakePtr<CompressorCode::Decoder<CompressorCode::Version4> >();
  }

  Decoder::Ptr CreateCompressorCode4PlusDecoder()
  {
    return MakePtr<CompressorCode::Decoder<CompressorCode::Version4Plus> >();
  }
}  // namespace Formats::Packed
