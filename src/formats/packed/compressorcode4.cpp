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

//local includes
#include "formats/packed/container.h"
#include "formats/packed/pack_utils.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
//std includes
#include <algorithm>
#include <iterator>
//text includes
#include <formats/text/packed.h>

namespace Formats
{
namespace Packed
{
  namespace CompressorCode
  {
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    struct Version4
    {
      static const StringView DESCRIPTION;
      static const std::size_t MIN_SIZE = 256;//TODO
      static const StringView DEPACKER_PATTERN;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
      PACK_PRE struct RawHeader
      {
        //+0
        uint8_t Padding1[0x0e];
        //+0x0e
        uint16_t RestDepackerSize;
        //+0x10
        uint8_t Padding2[0x0a];
        //+0x1a
        uint16_t ChunksCount;
        //+0x1c
      } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

      static bool FastCheck(const RawHeader& header, std::size_t size)
      {
        const uint_t depackerSize = 0x14 + fromLE(header.RestDepackerSize);
        const uint_t chunksCount = fromLE(header.ChunksCount);
        //at least one byte per chunk
        if (chunksCount > MAX_DECODED_SIZE ||
            depackerSize + chunksCount > size)
        {
          return false;
        }
        const uint8_t RET_CODE = 0xc9;
        const uint8_t* const depacker = safe_ptr_cast<const uint8_t*>(&header);
        return depacker[depackerSize - 1] == RET_CODE;
      }

      static_assert(sizeof(RawHeader) == 0x1c, "Invalid layout");
    };

    struct Version4Plus
    {
      static const StringView DESCRIPTION;
      static const std::size_t MIN_SIZE = 256;//TODO
      static const StringView DEPACKER_PATTERN;

  #ifdef USE_PRAGMA_PACK
  #pragma pack(push,1)
  #endif
      PACK_PRE struct RawHeader
      {
        //+0
        uint8_t Padding1[0x0e];
        //+0x0e
        uint16_t RestDepackerSize;
        //+0x10
        uint8_t Padding2[0x0d];
        //+0x1d
        uint16_t PackedDataSize;
        //+0x1f
        uint8_t Padding3[0x15];
        //+0x34
        uint16_t ChunksCount;
        //+0x36
      } PACK_POST;
  #ifdef USE_PRAGMA_PACK
  #pragma pack(pop)
  #endif

      static bool FastCheck(const RawHeader& header, std::size_t size)
      {
        const uint_t depackerSize = 0x14 + fromLE(header.RestDepackerSize);
        const uint_t chunksCount = fromLE(header.ChunksCount);
        //at least one byte per chunk
        if (chunksCount > MAX_DECODED_SIZE ||
            depackerSize > size ||
            !fromLE(header.PackedDataSize) ||
            depackerSize < 257)
        {
          return false;
        }
        return *(header.Padding1 + depackerSize - 256 - 1) == 0xc9;
      }

      static_assert(sizeof(RawHeader) == 0x36, "Invalid layout");
    };

    const StringView Version4::DESCRIPTION = Text::CC4_DECODER_DESCRIPTION;
    const StringView Version4::DEPACKER_PATTERN(
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
      //add hl,bc points here +14
      "fde5"    // push iy
      "11??"    // ld de,xxxx ;dst addr
      "01??"    // ld bc,xxxx ;packed size +1a
      "c5"      // push bc
      "01??"    // ld bc,xxxx
      "c5"      // push bc
    );

    const StringView Version4Plus::DESCRIPTION = Text::CC4PLUS_DECODER_DESCRIPTION;
    const StringView Version4Plus::DEPACKER_PATTERN(
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
      //add hl,bc points here +14
      "fde5"    // push iy
      "e5"      // push hl
      "dde1"    // pop ix
      "11??"    // ld de,xxxx ;dst addr
      "01??"    // ld bc,xxxx ;huffman packed size +1d
    );

    template<class Version>
    class Container
    {
    public:
      Container(const void* data, std::size_t size)
        : Data(static_cast<const uint8_t*>(data))
        , Size(size)
      {
      }

      bool FastCheck() const
      {
        if (Size < sizeof(typename Version::RawHeader))
        {
          return false;
        }
        const typename Version::RawHeader& header = GetHeader();
        return Version::FastCheck(header, Size);
      }

      std::size_t GetAvailableData() const
      {
        return Size;
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

    class StreamAdapter : public ByteStream
    {
    public:
      StreamAdapter(const uint8_t* data, std::size_t size)
        : ByteStream(data, size)
        , UsedData(0)
      {
      }

      std::size_t GetUsedData() const
      {
        return UsedData;
      }

      uint8_t operator * ()
      {
        Require(!Eof());
        ++UsedData;
        return GetByte();
      }

      StreamAdapter& operator ++ (int)
      {
        return *this;
      }

    private:
      std::size_t UsedData;
    };

    class RawDataDecoder
    {
    public:
      RawDataDecoder(const uint8_t* data, std::size_t size, uint_t chunksCount)
        : IsValid(true)
        , Stream(data, size)
        , ChunksCount(chunksCount)
        , Result(new Dump())
        , Decoded(*Result)
      {
        if (IsValid && !Stream.Eof())
        {
          IsValid = DecodeData();
        }
      }

      std::unique_ptr<Dump> GetResult()
      {
        return IsValid
          ? std::move(Result)
          : std::unique_ptr<Dump>();
      }

      std::size_t GetUsedSize() const
      {
        return Stream.GetUsedData();
      }
    private:
      bool DecodeData()
      {
        try
        {
          uint_t chunksCount = ChunksCount;
          Decoded.reserve(2 * chunksCount);
          StreamAdapter& source(Stream);
          std::back_insert_iterator<Dump> target(Decoded);
          //assume that first byte always exists due to header format
          while (chunksCount-- && Decoded.size() < MAX_DECODED_SIZE)
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
            uint_t len = (data & 1)
              ? 256 * count + *source
              : count + 3;
            switch (data & 0x1f)
            {
            case 0:
              len -= 2;
            case 1:
              while (len--)
              {
                *target = *source;
              }
              break;
            case 2:
              --len;
            case 3:
              std::fill_n(target, len, 0);
              break;
            case 4:
              --len;
            case 5:
              std::fill_n(target, len, 0xff);
              break;
            case 6:
            case 7:
              std::fill_n(target, len, *source);
              break;
            case 8:
              ++len;
            case 9:
              for (uint8_t data = *source, delta = *source; len; --len, data += delta)
              {
                *target = data;
              }
              break;
            case 0xa:
              --len;
            case 0xb:
              for (const uint8_t data1 = *source, data2 = *source; len; --len)
              {
                *target = data1;
                *target = data2;
              }
              break;
            case 0xc:
              --len;
            case 0xd:
              for (const uint8_t data1 = *source, data2 = *source, data3 = *source; len; --len)
              {
                *target = data1;
                *target = data2;
                *target = data3;
              }
              break;
            case 0xe:
            case 0xf:
              for (const uint8_t data = *source; len; --len)
              {
                  *target = data;
                  *target = *source;
              }
              break;
            case 0x10:
            case 0x11:
              for (const uint8_t data = *source; len; --len)
              {
                  *target = data;
                  *target = *source;
                  *target = *source;
              }
              break;
            case 0x12:
            case 0x13:
              for (const uint8_t base = *source; len; --len)
              {
                const uint8_t data = *source;
                *target = base + (data >> 4);
                *target = base + (data & 0x0f);
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
      bool IsValid;
      StreamAdapter Stream;
      const uint_t ChunksCount;
      std::unique_ptr<Dump> Result;
      Dump& Decoded;
    };

    template<class Version>
    class DataDecoder;

    template<>
    class DataDecoder<Version4>
    {
    public:
      explicit DataDecoder(const Container<Version4>& container)
        : Header(container.GetHeader())
        , DataOffset(0x14 + fromLE(Header.RestDepackerSize))
        , Delegate(container.FastCheck()
          ? new RawDataDecoder(Header.Padding1 + DataOffset, container.GetAvailableData() - DataOffset, fromLE(Header.ChunksCount))
          : nullptr)
      {
      }

      std::unique_ptr<Dump> GetResult()
      {
        return Delegate.get()
          ? Delegate->GetResult()
          : std::unique_ptr<Dump>();
      }

      std::size_t GetUsedSize() const
      {
        return Delegate.get()
          ? DataOffset + Delegate->GetUsedSize()
          : 0;
      }
    private:
      const Version4::RawHeader& Header;
      const std::size_t DataOffset;
      const std::unique_ptr<RawDataDecoder> Delegate;
    };


    class Bitstream
    {
    public:
      Bitstream(const uint8_t* data, std::size_t size)
        : Source(data, size)
        , Bits(), Mask(0)
      {
      }

      std::size_t GetUsedData() const
      {
        return Source.GetUsedData();
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
        if (uint_t len = GetBits(3))
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
      uint_t Bits;
      uint_t Mask;
    };

    template<>
    class DataDecoder<Version4Plus>
    {
    public:
      explicit DataDecoder(const Container<Version4Plus>& container)
        : Header(container.GetHeader())
        , DataOffset(0x14 + fromLE(Header.RestDepackerSize))
        , DataSize(0)
      {
        if (container.FastCheck() && DecodeHuffman(container.GetAvailableData() - DataOffset))
        {
          Delegate.reset(new RawDataDecoder(UnhuffmanData.data(), UnhuffmanData.size(), fromLE(Header.ChunksCount)));
        }
      }

      std::unique_ptr<Dump> GetResult()
      {
        return Delegate.get()
          ? Delegate->GetResult()
          : std::unique_ptr<Dump>();
      }

      std::size_t GetUsedSize() const
      {
        return Delegate.get()
          ? DataOffset + DataSize
          : 0;
      }
    private:
      bool DecodeHuffman(std::size_t availableSize)
      {
        try
        {
          Bitstream stream(Header.Padding1 + DataOffset, availableSize);
          const uint8_t* const table = Header.Padding1 + DataOffset - 256;
          for (uint_t packedBytes = fromLE(Header.PackedDataSize); packedBytes; --packedBytes)
          {
            const uint_t idx = stream.GetIndex();
            UnhuffmanData.push_back(table[idx]);
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
      std::size_t DataSize;
      Dump UnhuffmanData;
      std::unique_ptr<RawDataDecoder> Delegate;
    };
  }//namespace CompressorCode

  template<class Version>
  class CompressorCodeDecoder : public Decoder
  {
  public:
    CompressorCodeDecoder()
      : Depacker(Binary::CreateFormat(Version::DEPACKER_PATTERN, Version::MIN_SIZE))
    {
    }

    String GetDescription() const override
    {
      return Version::DESCRIPTION.to_string();
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Depacker;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Depacker->Match(rawData))
      {
        return Container::Ptr();
      }
      const CompressorCode::Container<Version> container(rawData.Start(), rawData.Size());
      if (!container.FastCheck())
      {
        return Container::Ptr();
      }
      CompressorCode::DataDecoder<Version> decoder(container);
      return CreateContainer(decoder.GetResult(), decoder.GetUsedSize());
    }
  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateCompressorCode4Decoder()
  {
    return MakePtr<CompressorCodeDecoder<CompressorCode::Version4> >();
  }

  Decoder::Ptr CreateCompressorCode4PlusDecoder()
  {
    return MakePtr<CompressorCodeDecoder<CompressorCode::Version4Plus> >();
  }
}//namespace Packed
}//namespace Formats
