/*
Abstract:
  ZXZip support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <crc.h>
#include <detector.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <cstring>
#include <stdexcept>
//text includes
#include <core/text/plugins.h>

namespace ZXZip
{
  const std::size_t MAX_DECODED_SIZE = 0xff00;
  //checkers
  const std::string HEADER_PATTERN =
    //Filename
    "20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a"
    //Type
    "20-7a ??"
    //SourceSize
    "??"
    //SourceSectors
    "01-ff"
    //PackedSize
    "??"
    //SourceCRC
    "????"
    //Method
    "00-03"
    //Flags
    "%0000000x"
  ;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0x0
    char Name[8];
    //+0x8
    char Type;
    //+0x9
    uint16_t StartOrSize;
    //+0xb
    uint16_t SourceSize;
    //+0xd
    uint8_t SourceSectors;
    //+0xe
    uint16_t PackedSize;
    //+0x10
    uint32_t SourceCRC;
    //+0x14
    uint8_t Method;
    //+0x15
    uint8_t Flags;
    //+0x16
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x16);

  uint_t GetSourceFileSize(const RawHeader& header)
  {
    const uint_t calcSize = header.Type == 'B' || header.Type == 'b'
      ? 4 + fromLE(header.StartOrSize)
      : fromLE(header.SourceSize);
    const uint_t calcSectors = align(calcSize, uint_t(256)) / 256;
    return calcSectors == header.SourceSectors
      ? calcSize
      : 256 * header.SourceSectors;
  }

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
      if (Size < sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      if (GetSourceFileSize(header) < fromLE(header.PackedSize))
      {
        return false;
      }
      return GetUsedSize() <= Size;
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return sizeof(header) + fromLE(header.PackedSize);
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

  enum PackMethods
  {
    STORE = 0,
    LZPRESS = 1,
    SHRUNK = 2,
    IMPLODE = 3
  };

  class DataDecoder
  {
  public:
    virtual ~DataDecoder() {}

    virtual Dump* GetDecodedData() = 0;
  };

  class StoreDataDecoder : public DataDecoder
  {
  public:
    explicit StoreDataDecoder(const RawHeader& header)
      : Header(header)
    {
      assert(STORE == Header.Method);
    }

    virtual Dump* GetDecodedData()
    {
      const uint_t packedSize = fromLE(Header.PackedSize);
      const uint8_t* const sourceData = safe_ptr_cast<const uint8_t*>(&Header + 1);
      Decoded.resize(packedSize);
      std::memcpy(&Decoded[0], sourceData, packedSize);
      return &Decoded;
    }
  private:
    const RawHeader& Header;
    Dump Decoded;
  };

  const uint8_t SFT_64_1[] =
  {
    0x07,//size
    0x01, 0x13, 0x34, 0xE5, 0xF6, 0x96, 0xF7 // packed data llllbbbb
  };

  const uint8_t SFT_64_2[] =
  {
   0x10,
   0x00, 0x12, 0x03, 0x24, 0x15, 0x36, 0x27,
   0x38, 0x39, 0x6A, 0x7B, 0x4C, 0x9D, 0x6E,
   0x1F, 0x09
  };

  const uint8_t SFT_64_3[] =
  {
   0x07,
   0x12, 0x23, 0x14, 0xE5, 0xF6, 0x96, 0xF7
  };

  const uint8_t SFT_64_4[] =
  {
   0x0D,
   0x01, 0x22, 0x23, 0x14, 0x15, 0x36, 0x37,
   0x68, 0x89, 0x9A, 0xDB, 0x3C, 0x05
  };

  const uint8_t SFT_64_5[] =
  {
   0x07,
   0x12, 0x13, 0x44, 0xC5, 0xF6, 0x96, 0xF7
  };

  const uint8_t SFT_64_6[] =
  {
   0x0E,
   0x02, 0x01, 0x12, 0x23, 0x14, 0x15, 0x36,
   0x37, 0x68, 0x89, 0x9A, 0xDB, 0x3C, 0x05
  };

  const uint8_t SFT_100[] =
  {
   0x62,
   0x0A, 0x7B, 0x07, 0x06, 0x1B, 0x06, 0xBB,
   0x0C, 0x4B, 0x03, 0x09, 0x07, 0x0B, 0x09,
   0x0B, 0x09, 0x07, 0x16, 0x07, 0x08, 0x06,
   0x05, 0x06, 0x07, 0x06, 0x05, 0x36, 0x07,
   0x16, 0x17, 0x0B, 0x0A, 0x06, 0x08, 0x0A,
   0x0B, 0x05, 0x06, 0x15, 0x04, 0x06, 0x17,
   0x05, 0x0A, 0x08, 0x05, 0x06, 0x15, 0x06,
   0x0A, 0x25, 0x06, 0x08, 0x07, 0x18, 0x0A,
   0x07, 0x0A, 0x08, 0x0B, 0x07, 0x0B, 0x04,
   0x25, 0x04, 0x25, 0x04, 0x0A, 0x06, 0x04,
   0x05, 0x14, 0x05, 0x09, 0x34, 0x07, 0x06,
   0x17, 0x09, 0x1A, 0x2B, 0xFC, 0xFC, 0xFC,
   0xFB, 0xFB, 0xFB, 0x0C, 0x0B, 0x2C, 0x0B,
   0x2C, 0x0B, 0x3C, 0x0B, 0x2C, 0x2B, 0xAC
  };

  struct SFTEntry
  {
    uint_t Bits;
    uint_t Value;
    uint_t Code;

    bool operator < (const SFTEntry& rh) const
    {
      return Bits == rh.Bits 
        ? Value < rh.Value
        : Bits < rh.Bits;
    }
  };

  //implode bitstream decoder
  class SafeByteStream : public ByteStream
  {
  public:
    SafeByteStream(const uint8_t* data, std::size_t size)
      : ByteStream(data, size)
    {
    }

    uint8_t GetByte()
    {
      if (Eof())
      {
        throw std::exception();
      }
      return ByteStream::GetByte();
    }
  };

  class Bitstream : public SafeByteStream
  {
  public:
    Bitstream(const uint8_t* data, std::size_t size)
      : SafeByteStream(data, size)
      , Bits(), Mask(128)
    {
    }

    uint_t GetBit()
    {
      if ((Mask <<= 1) > 255)
      {
        Bits = GetByte();
        Mask = 0x1;
      }
      return Bits & Mask ? 1 : 0;
    }

    uint_t GetBits(unsigned count)
    {
      uint_t result = 0;
      for (uint_t idx = 0; idx < count; ++idx)
      {
        result = result | (GetBit() << idx);
      }
      return result;
    }

    uint_t ReadByTree(const std::vector<SFTEntry>& tree)
    {
      std::vector<SFTEntry>::const_iterator it = tree.begin();
      for (uint_t bits = 0, result = 0; ;)
      {
        result |= GetBit() << bits++;
        for (; it->Bits <= bits; ++it)
        {
          if (it == tree.end())
          {
            throw std::exception();
          }
          if (it->Bits == bits && it->Code == result)
          {
            return it->Value;
          }
        }
      }
      return 0;
    }
  private:
    uint_t Bits;
    uint_t Mask;
  };

  class ImplodeDataDecoder : public DataDecoder
  {
  public:
    explicit ImplodeDataDecoder(const RawHeader& header)
      : Header(header)
    {
      assert(IMPLODE == Header.Method);
    }

    virtual Dump* GetDecodedData()
    {
      const uint_t dataSize = GetSourceFileSize(Header);
      const bool isBigFile = dataSize >= 0x1600;
      const bool isTextFile = 0 != (Header.Flags & 1);
      const bool isBigTextFile = isBigFile && isTextFile;

      std::vector<SFTEntry> SFT1;
      std::vector<SFTEntry> SFT2;
      std::vector<SFTEntry> SFT3;

      if (isBigTextFile)
      {
        ExtractTree(SFT_100, SFT1);
        assert(SFT1.size() == 256);
      }
      if (isTextFile)
      {
        ExtractTree(isBigFile ? SFT_64_4 : SFT_64_6, SFT2);
        ExtractTree(isBigFile ? SFT_64_3 : SFT_64_5, SFT3);
      }
      else
      {
        ExtractTree(SFT_64_2, SFT2);
        ExtractTree(SFT_64_1, SFT3);
      }
      assert(SFT2.size() == 64);
      assert(SFT3.size() == 64);

      try
      {
        const uint_t distBits = isBigTextFile ? 7 : 6;
        const uint_t minMatchLen = isBigTextFile ? 3 : 2;

        Bitstream stream(safe_ptr_cast<const uint8_t*>(&Header + 1), fromLE(Header.PackedSize));
        Dump result;
        while (!stream.Eof())
        {
          if (stream.GetBit())
          {
            const uint_t data = isBigTextFile
              ? stream.ReadByTree(SFT1)
              : stream.GetBits(8);
            result.push_back(data);
          }
          else
          {
            const uint_t loDist = stream.GetBits(distBits);
            const uint_t hiDist = stream.ReadByTree(SFT3);
            const uint_t dist = (loDist | (hiDist << distBits)) + 1;

            uint_t len = stream.ReadByTree(SFT2);
            if (63 == len)
            {
              len += stream.GetBits(8);
            }
            len += minMatchLen;
            if (dist > result.size())
            {
              const uint_t zeroes = std::min(dist - result.size(), len);
              std::fill_n(std::back_inserter(result), zeroes, 0);
              len -= zeroes;
            }
            if (len)
            {
              CopyFromBack(dist, result, len);
            }
          }
        }
        Decoded.swap(result);
        return &Decoded;
      }
      catch (const std::exception&)
      {
        return 0;
      }
    }
  private:
    static uint_t InvertBits(uint_t val)
    {
      uint_t res = 0;
      for (uint_t count = 16, mask = val; count; --count, mask >>= 1)
      {
        res = 2 * res + (mask & 1);
      }
      return res;
    }

    static void ExtractTree(const uint8_t* data, std::vector<SFTEntry>& result)
    {
      std::vector<SFTEntry> tree;
      for (uint_t count = *data++, idx = 0; count; --count)
      {
        const uint_t bits = (*data & 0x0f) + 1;
        for (uint_t groupLen = (*data++ >> 4) + 1; groupLen; --groupLen, ++idx)
        {
          const SFTEntry entry = {bits, idx, 0};
          tree.push_back(entry);
        }
      }
      std::sort(tree.begin(), tree.end());
      assert(InvertBits(1) == 0x8000);
      assert(InvertBits(0x180) == 0x180);
      assert(InvertBits(0x8000) == 1);
      uint_t code = 0, codeIncrement = 0, lastBits = 0;
      for (std::vector<SFTEntry>::reverse_iterator it = tree.rbegin(), lim = tree.rend(); it != lim; ++it)
      {
        code += codeIncrement;
        it->Code = InvertBits(code);
        if (it->Bits != lastBits)
        {
          lastBits = it->Bits;
          codeIncrement = 1 << (16 - lastBits);
        }
      }
      result.swap(tree);
    }
  private:
    const RawHeader& Header;
    Dump Decoded;
  };

  std::auto_ptr<DataDecoder> CreateDecoder(const RawHeader& header)
  {
    switch (header.Method)
    {
    case STORE:
      return std::auto_ptr<DataDecoder>(new StoreDataDecoder(header));
    case IMPLODE:
      return std::auto_ptr<DataDecoder>(new ImplodeDataDecoder(header));
    default:
      assert(!"Unsupported ZXZip packing method");
      return std::auto_ptr<DataDecoder>();
    };
  };

  class DispatchedDataDecoder
  {
  public:
    explicit DispatchedDataDecoder(const Container& container)
      : Header(container.GetHeader())
      , Delegate(CreateDecoder(Header))
      , IsValid(container.FastCheck() && Delegate.get())
    {
    }

    Dump* GetDecodedData()
    {
      if (!IsValid)
      {
        return 0;
      }
      while (Dump* result = Delegate->GetDecodedData())
      {
        const uint_t dataSize = GetSourceFileSize(Header);
        if (dataSize != result->size())
        {
          break;
        }
        const uint32_t realCRC = Crc32(&result->front(), result->size());
        //ZXZip CRC32 calculation does not invert result
        if (realCRC != ~fromLE(Header.SourceCRC))
        {
          break;
        }
        return result;
      }
      IsValid = false;
      return 0;
    }
  private:
    const RawHeader& Header;
    const std::auto_ptr<DataDecoder> Delegate;
    bool IsValid;
  };
}

namespace Formats
{
  namespace Packed
  {
    class ZXZipDecoder : public Decoder
    {
    public:
      ZXZipDecoder()
        : Depacker(DataFormat::Create(ZXZip::HEADER_PATTERN))
      {
      }

      virtual DataFormat::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const ZXZip::Container container(data, availSize);
        return container.FastCheck() && Depacker->Match(data, availSize);
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const ZXZip::Container container(data, availSize);
        if (!container.FastCheck() || !Depacker->Match(data, availSize))
        {
          return std::auto_ptr<Dump>();
        }
        ZXZip::DispatchedDataDecoder decoder(container);
        if (Dump* decoded = decoder.GetDecodedData())
        {
          usedSize = container.GetUsedSize();
          std::auto_ptr<Dump> res(new Dump());
          res->swap(*decoded);
          return res;
        }
        return std::auto_ptr<Dump>();
      }
    private:
      const DataFormat::Ptr Depacker;
    };

    Decoder::Ptr CreateZXZipDecoder()
    {
      return Decoder::Ptr(new ZXZipDecoder());
    }
  }
}
