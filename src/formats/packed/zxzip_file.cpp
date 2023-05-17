/**
 *
 * @file
 *
 * @brief  ZXZip compressor support
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
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/crc.h>
#include <binary/format_factories.h>
#include <formats/packed.h>
#include <math/numeric.h>
// std includes
#include <array>
#include <cstring>
#include <iterator>

namespace Formats::Packed
{
  namespace ZXZip
  {
    const std::size_t MIN_SIZE = 0x16 + 32;

    const Char DESCRIPTION[] = "ZXZip";
    const auto HEADER_PATTERN =
        // Filename
        "20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a"
        // Type
        "20-7a ??"
        // SourceSize
        "??"
        // SourceSectors
        "01-ff"
        // PackedSize
        "??"
        // SourceCRC
        "????"
        // Method
        "00-03"
        // Flags
        "%0000000x"
        ""_sv;

    struct RawHeader
    {
      //+0x0
      char Name[8];
      //+0x8
      char Type;
      //+0x9
      le_uint16_t StartOrSize;
      //+0xb
      le_uint16_t SourceSize;
      //+0xd
      uint8_t SourceSectors;
      //+0xe
      le_uint16_t PackedSize;
      //+0x10
      le_uint32_t SourceCRC;
      //+0x14
      uint8_t Method;
      //+0x15
      uint8_t Flags;
      //+0x16
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x16, "Invalid layout");

    std::size_t GetSourceFileSize(const RawHeader& header)
    {
      const uint_t calcSize = header.Type == 'B' || header.Type == 'b' ? uint_t(4 + header.StartOrSize)
                                                                       : uint_t(header.SourceSize);
      const std::size_t calcSectors = Math::Align(calcSize, uint_t(256)) / 256;
      return calcSectors == header.SourceSectors ? calcSize : 256 * header.SourceSectors;
    }

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
        const std::size_t packedSize = header.PackedSize;
        if (!packedSize)
        {
          return false;
        }
        if (GetSourceFileSize(header) < packedSize)
        {
          return false;
        }
        return GetUsedSize() <= Size;
      }

      std::size_t GetUsedSize() const
      {
        const RawHeader& header = GetHeader();
        return sizeof(header) + header.PackedSize;
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
      SHRINK = 2,
      IMPLODE = 3
    };

    class DataDecoder
    {
    public:
      virtual ~DataDecoder() = default;

      virtual Binary::Container::Ptr GetDecodedData() = 0;
    };

    class StoreDataDecoder : public DataDecoder
    {
    public:
      explicit StoreDataDecoder(const RawHeader& header)
        : Header(header)
      {
        assert(STORE == Header.Method);
      }

      Binary::Container::Ptr GetDecodedData() override
      {
        const uint_t packedSize = Header.PackedSize;
        const auto* const sourceData = safe_ptr_cast<const uint8_t*>(&Header + 1);
        return Binary::CreateContainer(Binary::View{sourceData, packedSize});
      }

    private:
      const RawHeader& Header;
    };

    const uint8_t SFT_64_1[] = {
        0x07,                                     // size
        0x01, 0x13, 0x34, 0xE5, 0xF6, 0x96, 0xF7  // packed data llllbbbb
    };

    const uint8_t SFT_64_2[] = {0x10, 0x00, 0x12, 0x03, 0x24, 0x15, 0x36, 0x27, 0x38,
                                0x39, 0x6A, 0x7B, 0x4C, 0x9D, 0x6E, 0x1F, 0x09};

    const uint8_t SFT_64_3[] = {0x07, 0x12, 0x23, 0x14, 0xE5, 0xF6, 0x96, 0xF7};

    const uint8_t SFT_64_4[] = {0x0D, 0x01, 0x22, 0x23, 0x14, 0x15, 0x36, 0x37, 0x68, 0x89, 0x9A, 0xDB, 0x3C, 0x05};

    const uint8_t SFT_64_5[] = {0x07, 0x12, 0x13, 0x44, 0xC5, 0xF6, 0x96, 0xF7};

    const uint8_t SFT_64_6[] = {0x0E, 0x02, 0x01, 0x12, 0x23, 0x14, 0x15, 0x36,
                                0x37, 0x68, 0x89, 0x9A, 0xDB, 0x3C, 0x05};

    const uint8_t SFT_100[] = {0x62, 0x0A, 0x7B, 0x07, 0x06, 0x1B, 0x06, 0xBB, 0x0C, 0x4B, 0x03, 0x09, 0x07, 0x0B, 0x09,
                               0x0B, 0x09, 0x07, 0x16, 0x07, 0x08, 0x06, 0x05, 0x06, 0x07, 0x06, 0x05, 0x36, 0x07, 0x16,
                               0x17, 0x0B, 0x0A, 0x06, 0x08, 0x0A, 0x0B, 0x05, 0x06, 0x15, 0x04, 0x06, 0x17, 0x05, 0x0A,
                               0x08, 0x05, 0x06, 0x15, 0x06, 0x0A, 0x25, 0x06, 0x08, 0x07, 0x18, 0x0A, 0x07, 0x0A, 0x08,
                               0x0B, 0x07, 0x0B, 0x04, 0x25, 0x04, 0x25, 0x04, 0x0A, 0x06, 0x04, 0x05, 0x14, 0x05, 0x09,
                               0x34, 0x07, 0x06, 0x17, 0x09, 0x1A, 0x2B, 0xFC, 0xFC, 0xFC, 0xFB, 0xFB, 0xFB, 0x0C, 0x0B,
                               0x2C, 0x0B, 0x2C, 0x0B, 0x3C, 0x0B, 0x2C, 0x2B, 0xAC};

    struct SFTEntry
    {
      uint_t Bits;
      uint_t Value;
      uint_t Code;

      bool operator<(const SFTEntry& rh) const
      {
        return Bits == rh.Bits ? Value < rh.Value : Bits < rh.Bits;
      }
    };

    // implode bitstream decoder
    class SafeByteStream : public ByteStream
    {
    public:
      SafeByteStream(const uint8_t* data, std::size_t size)
        : ByteStream(data, size)
      {}

      uint8_t GetByte()
      {
        Require(!Eof());
        return ByteStream::GetByte();
      }
    };

    class Bitstream : public SafeByteStream
    {
    public:
      Bitstream(const uint8_t* data, std::size_t size)
        : SafeByteStream(data, size)
        , Bits()
        , Mask(128)
      {}

      bool GetBit()
      {
        if ((Mask <<= 1) > 255)
        {
          Bits = GetByte();
          Mask = 0x1;
        }
        return 0 != (Bits & Mask);
      }

      uint_t GetBits(uint_t count)
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
        auto it = tree.begin();
        for (uint_t bits = 0, result = 0;;)
        {
          result |= GetBit() << bits++;
          for (; it->Bits <= bits; ++it)
          {
            Require(it != tree.end());
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

      Binary::Container::Ptr GetDecodedData() override
      {
        const std::size_t dataSize = GetSourceFileSize(Header);
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

          Bitstream stream(safe_ptr_cast<const uint8_t*>(&Header + 1), Header.PackedSize);
          Binary::DataBuilder result(2 * Header.PackedSize);
          while (!stream.Eof())
          {
            if (stream.GetBit())
            {
              const uint_t data = isBigTextFile ? stream.ReadByTree(SFT1) : stream.GetBits(8);
              result.AddByte(static_cast<uint8_t>(data));
            }
            else
            {
              const uint_t loDist = stream.GetBits(distBits);
              const uint_t hiDist = stream.ReadByTree(SFT3);
              const std::size_t dist = (loDist | (hiDist << distBits)) + 1;

              std::size_t len = stream.ReadByTree(SFT2);
              if (63 == len)
              {
                len += stream.GetBits(8);
              }
              len += minMatchLen;
              if (dist > result.Size())
              {
                const std::size_t zeroes = std::min<std::size_t>(dist - result.Size(), len);
                Fill(result, zeroes, 0);
                len -= zeroes;
              }
              if (len)
              {
                CopyFromBack(dist, result, len);
              }
            }
          }
          return result.CaptureResult();
        }
        catch (const std::exception&)
        {
          return {};
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
        for (auto it = tree.rbegin(), lim = tree.rend(); it != lim; ++it)
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
    };

    struct LZWEntry
    {
      static const uint_t LIMITER = 256;

      uint_t Parent = 0;
      uint8_t Value = '\0';
      bool IsFree = false;

      LZWEntry() = default;

      explicit LZWEntry(uint_t value)
        : Parent(LIMITER)
        , Value(static_cast<uint8_t>(value < LIMITER ? value : 0))
        , IsFree(value >= LIMITER)
      {}
    };

    using LZWTree = std::array<LZWEntry, 8192>;

    class ShrinkDataDecoder : public DataDecoder
    {
    public:
      explicit ShrinkDataDecoder(const RawHeader& header)
        : Header(header)
      {
        assert(SHRINK == Header.Method);
      }

      Binary::Container::Ptr GetDecodedData() override
      {
        try
        {
          Bitstream stream(safe_ptr_cast<const uint8_t*>(&Header + 1), Header.PackedSize);
          Binary::DataBuilder result(2 * Header.PackedSize);

          LZWTree tree;
          ResetTree(tree);

          auto lastFree = tree.begin() + LZWEntry::LIMITER;

          uint_t codeSize = 9;
          uint_t oldCode = stream.GetBits(codeSize);
          result.AddByte(static_cast<uint8_t>(oldCode));
          while (!stream.Eof())
          {
            const uint_t code = stream.GetBits(codeSize);
            if (LZWEntry::LIMITER == code)
            {
              const uint_t subCode = stream.GetBits(codeSize);
              if (1 == subCode)
              {
                ++codeSize;
              }
              else if (2 == subCode)
              {
                PartialResetTree(tree);
                lastFree = tree.begin() + LZWEntry::LIMITER;
              }
            }
            else
            {
              const bool isFree = tree.at(code).IsFree;
              Binary::Dump substring(isFree ? 1 : 0);
              for (uint_t curCode = isFree ? oldCode : code; curCode != LZWEntry::LIMITER;)
              {
                const LZWEntry& curEntry = tree.at(curCode);
                Require(curCode != curEntry.Parent);
                Require(substring.size() <= tree.size());
                substring.push_back(curEntry.Value);
                curCode = curEntry.Parent;
              }
              Require(!substring.empty());
              if (isFree)
              {
                substring.front() = substring.back();
              }
              std::reverse(substring.begin(), substring.end());
              result.Add(substring);
              for (++lastFree; lastFree != tree.end() && !lastFree->IsFree; ++lastFree)
              {}
              Require(lastFree != tree.end());
              lastFree->Value = substring.front();
              lastFree->Parent = oldCode;
              lastFree->IsFree = false;
              oldCode = code;
            }
          }
          return result.CaptureResult();
        }
        catch (const std::exception&)
        {
          return {};
        }
      }

    private:
      static void ResetTree(LZWTree& tree)
      {
        for (uint_t idx = 0; idx < tree.size(); ++idx)
        {
          tree[idx] = LZWEntry(idx);
        }
      }

      static void PartialResetTree(LZWTree& tree)
      {
        const std::size_t parentsCount = tree.size() - LZWEntry::LIMITER;
        std::vector<bool> hasChilds(parentsCount);
        for (std::size_t idx = 0; idx < parentsCount; ++idx)
        {
          if (!tree[idx + LZWEntry::LIMITER].IsFree && tree[idx + LZWEntry::LIMITER].Parent > LZWEntry::LIMITER)
          {
            const std::size_t parent = tree[idx + LZWEntry::LIMITER].Parent - LZWEntry::LIMITER;
            hasChilds[parent] = true;
          }
        }
        for (std::size_t idx = 0; idx < parentsCount; ++idx)
        {
          if (!hasChilds[idx])
          {
            tree[idx + LZWEntry::LIMITER].IsFree = true;
            tree[idx + LZWEntry::LIMITER].Parent = LZWEntry::LIMITER;
          }
        }
      }

    private:
      const RawHeader& Header;
    };

    std::unique_ptr<DataDecoder> CreateDecoder(const RawHeader& header)
    {
      switch (header.Method)
      {
      case STORE:
        return std::unique_ptr<DataDecoder>(new StoreDataDecoder(header));
      case IMPLODE:
        return std::unique_ptr<DataDecoder>(new ImplodeDataDecoder(header));
      case SHRINK:
        return std::unique_ptr<DataDecoder>(new ShrinkDataDecoder(header));
      default:
        return std::unique_ptr<DataDecoder>();
      };
    };

    class DispatchedDataDecoder : public DataDecoder
    {
    public:
      explicit DispatchedDataDecoder(const Container& container)
        : Header(container.GetHeader())
        , Delegate(CreateDecoder(Header))
        , IsValid(container.FastCheck() && Delegate.get())
      {}

      Binary::Container::Ptr GetDecodedData() override
      {
        if (!IsValid)
        {
          return {};
        }
        auto result = Delegate->GetDecodedData();
        while (result.get())
        {
          const std::size_t dataSize = GetSourceFileSize(Header);
          if (dataSize != result->Size())
          {
            break;
          }
          const uint32_t realCRC = Binary::Crc32(*result);
          // ZXZip CRC32 calculation does not invert result
          if (realCRC != ~Header.SourceCRC)
          {
            break;
          }
          return result;
        }
        IsValid = false;
        return {};
      }

    private:
      const RawHeader& Header;
      const std::unique_ptr<DataDecoder> Delegate;
      bool IsValid;
    };
  }  // namespace ZXZip

  class ZXZipDecoder : public Decoder
  {
  public:
    ZXZipDecoder()
      : Depacker(Binary::CreateFormat(ZXZip::HEADER_PATTERN, ZXZip::MIN_SIZE))
    {}

    String GetDescription() const override
    {
      return ZXZip::DESCRIPTION;
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
      const ZXZip::Container container(rawData.Start(), rawData.Size());
      if (!container.FastCheck())
      {
        return Container::Ptr();
      }
      ZXZip::DispatchedDataDecoder decoder(container);
      return CreateContainer(decoder.GetDecodedData(), container.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateZXZipDecoder()
  {
    return MakePtr<ZXZipDecoder>();
  }
}  // namespace Formats::Packed
