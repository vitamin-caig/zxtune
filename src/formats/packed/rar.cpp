/*
Abstract:
  Rar support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "rar_supp.h"
#include "pack_utils.h"
//common includes
#include <detector.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <cassert>
#include <memory>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>

namespace Rar
{
  const std::string THIS_MODULE("Formats::Packed::Rar");

  const std::string HEADER_PATTERN =
    "??"          // uint16_t CRC;
    "74"          // uint8_t Type;
    "?%1xxxxxxx"  // uint16_t Flags;
  ;

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
      if (Size < sizeof(Formats::Packed::Rar::FileBlockHeader))
      {
        return false;
      }
      const Formats::Packed::Rar::FileBlockHeader& header = GetHeader();
      if (header.IsBigFile() && Size < sizeof(Formats::Packed::Rar::BigFileBlockHeader))
      {
        return false;
      }
      if (!header.IsValid() || !header.IsSupported())
      {
        return false;
      }
      return GetUsedSize() <= Size;
    }

    std::size_t GetUsedSize() const
    {
      const Formats::Packed::Rar::FileBlockHeader& header = GetHeader();
      uint64_t res = fromLE(header.Size);
      res += fromLE(header.AdditionalSize);
      if (header.IsBigFile())
      {
        const Formats::Packed::Rar::BigFileBlockHeader& bigFile = *safe_ptr_cast<const Formats::Packed::Rar::BigFileBlockHeader*>(Data);
        res += uint64_t(fromLE(bigFile.PackedSizeHi)) << (8 * sizeof(uint32_t));
      }
      return static_cast<std::size_t>(res);
    }

    const Formats::Packed::Rar::FileBlockHeader& GetHeader() const
    {
      assert(Size >= sizeof(Formats::Packed::Rar::FileBlockHeader));
      return *safe_ptr_cast<const Formats::Packed::Rar::FileBlockHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };

  class CompressedFile
  {
  public:
    typedef std::auto_ptr<const CompressedFile> Ptr;
    virtual ~CompressedFile() {}

    virtual std::auto_ptr<Dump> Decompress() const = 0;
  };

  class NoCompressedFile : public CompressedFile
  {
  public:
    NoCompressedFile(const uint8_t* const start, std::size_t size, std::size_t destSize)
      : Start(start)
      , Size(size)
      , DestSize(destSize)
    {
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      if (Size != DestSize)
      {
        Log::Debug(THIS_MODULE, "Stored file sizes mismatch");
        return std::auto_ptr<Dump>();
      }
      else
      {
        Log::Debug(THIS_MODULE, "Restore");
        return std::auto_ptr<Dump>(new Dump(Start, Start + DestSize));
      }
    }
  private:
    const uint8_t* const Start;
    const std::size_t Size;
    const std::size_t DestSize;
  };

  const uint_t NC = 298;
  const uint_t DC = 48;
  const uint_t RC = 28;
  const uint_t BC = 19;
  const uint_t MC = 257;

  struct BaseDecodeTree
  {
    boost::array<uint_t, 16> DecodeLen;
    boost::array<uint_t, 16> DecodePos;

    BaseDecodeTree()
      : DecodeLen()
      , DecodePos()
    {
    }

    uint_t GetBitsForLen(uint_t len) const
    {
      uint_t bits = 1;
      while (bits < 15 && len >= DecodeLen[bits])
      {
        ++bits;
      }
      return bits;
    }
  };

  template<uint_t numSize>
  struct DecodeTree : BaseDecodeTree
  {
    boost::array<uint_t, numSize> DecodeNum;

    DecodeTree()
      : BaseDecodeTree()
      , DecodeNum()
    {
    }
  };

  typedef DecodeTree<RC> RepDecodeTree;
  typedef DecodeTree<BC> BitDecodeTree;
  typedef DecodeTree<NC> LitDecodeTree;
  typedef DecodeTree<DC> DistDecodeTree;
  typedef DecodeTree<MC> MultDecodeTree;

  class RarBitstream : public ByteStream
  {
  public:
    RarBitstream(const uint8_t* data, std::size_t size)
      : ByteStream(data, size)
      , ValidBits()
      , BitsBuffer()
    {
      FetchBits(0);
    }

    uint32_t ReadBits(uint_t bits)
    {
      const uint32_t result = PeekBits(bits);
      FetchBits(bits);
      return result;
    }

    template<uint_t numSize>
    uint_t DecodeNumber(const DecodeTree<numSize>& tree)
    {
      const uint_t idx = DecodeNumIndex(tree);
      return idx < numSize
        ? tree.DecodeNum[idx]
        : 0;
    }
  private:
    uint32_t PeekBits(uint_t bits)
    {
      static const boost::array<uint32_t, 17> BitMask =
      {
        {
          0,
          1, 3, 7, 15, 31, 63, 127, 255,
          511, 1023, 2047, 4095, 8191, 16383, 32767, 65535
        }
      };
      assert(ValidBits >= bits);
      assert(bits < BitMask.size());
      return BitMask[bits] & (BitsBuffer >> (ValidBits - bits));
    }

    void FetchBits(uint_t bits)
    {
      ValidBits -= bits;
      while (ValidBits < 24)
      {
        BitsBuffer = (BitsBuffer << 8) | GetByte();
        ValidBits += 8;
      }
    }

    uint_t DecodeNumIndex(const BaseDecodeTree& tree)
    {
      const uint_t len = PeekBits(16) & 0xfffe;
      const uint_t bits = tree.GetBitsForLen(len);
      FetchBits(bits);
      return tree.DecodePos[bits] + ((len - tree.DecodeLen[bits - 1]) >> (16 - bits));
    }
  private:
    uint32_t ValidBits;
    uint32_t BitsBuffer;
  };

  class DecodeTarget
  {
  public:
    DecodeTarget()
      : Cursor(Result.end())
      , Rest(0)
    {
    }

    void Reserve(std::size_t size)
    {
      const std::size_t prevSize = Result.size();
      Result.resize(prevSize + size);
      Cursor = Result.begin() + prevSize;
      Rest = size;
    }

    bool Full() const
    {
      return 0 == Rest;
    }

    void Add(uint8_t data)
    {
      *Cursor = data;
      ++Cursor;
      --Rest;
    }

    bool CopyFromBack(uint_t offset, uint_t count)
    {
      if (offset > static_cast<uint_t>(Cursor - Result.begin()) || count > Rest)
      {
        return false;
      }
      const Dump::const_iterator srcStart = Cursor - offset;
      const Dump::const_iterator srcEnd = srcStart + count;
      RecursiveCopy(srcStart, srcEnd, Cursor);
      Cursor += count;
      Rest -= count;
      return true;
    }

    Dump* Get()
    {
      return &Result;
    }
  private:
    Dump Result;
    Dump::iterator Cursor;
    std::size_t Rest;
  };

  class RarDecoder
  {
  public:
    RarDecoder(const uint8_t* const start, std::size_t size, std::size_t destSize)
      : Stream(start, size)
    {
      Decoded.Reserve(destSize);
    }

    Dump* GetDecodedData()
    {
      if (!Decoded.Full())
      {
        Decode();
      }
      return Decoded.Get();
    }
  private:
    void Decode()
    {
      ReadTables();
      while (!Decoded.Full())
      {
        if (Audio.Unpack)
        {
          DecodeAudioBlock();
        }
        else
        {
          DecodeDataBlock();
        }
      }
      ReadLastTables();
    }

    void ReadTables()
    {
      const uint_t flags = Stream.ReadBits(2);
      Audio.Unpack = 0 != (flags & 2);
      if (0 == (flags & 1))
      {
        UnpOldTable.assign(0);
      }

      if (Audio.Unpack)
      {
        const uint_t channels = Stream.ReadBits(2) + 1;
        Audio.SetChannels(channels);
      }
      const uint_t tableSize = Audio.Unpack
        ? MC * Audio.Channels
        : NC + DC + RC;

      boost::array<uint_t, BC> bitCount =  { {0} };
      for (uint_t i = 0; i < bitCount.size(); ++i)
      {
        bitCount[i] = Stream.ReadBits(4);
      }
      MakeDecodeTable(&bitCount[0], BD);
      boost::array<uint_t, MC * 4> table = { {0} };
      assert(tableSize <= table.size());
      for (uint_t i = 0; i < tableSize; )
      {
        const uint_t number = Stream.DecodeNumber(BD);
        if (number < 16)
        {
          table[i] = (number + UnpOldTable[i]) & 0xf;
          ++i;
        }
        else if (number == 16)
        {
          uint_t n = Stream.ReadBits(2) + 3;
          if (!i)
          {
            ++i;
            --n;
          }
          for (; n && i < tableSize; ++i, --n)
          {
            table[i] = table[i - 1];
          }
        }
        else
        {
          uint_t n = 17 == number
            ? (Stream.ReadBits(3) + 3)
            : (Stream.ReadBits(7) + 11);
          for (; n && i < tableSize; ++i, --n)
          {
            table[i] = 0;
          }
        }
      }

      if (Audio.Unpack)
      {
        for (uint_t i = 0; i < Audio.Channels; ++i)
        {
          MakeDecodeTable(&table[i * MC], MD[i]);
        }
      }
      else
      {
        MakeDecodeTable(&table[0], LD);
        MakeDecodeTable(&table[NC], DD);
        MakeDecodeTable(&table[NC + DC], RD);
      }
      UnpOldTable = table;
    }

    void ReadLastTables()
    {
      if (Audio.Unpack)
      {
        const uint_t num = Stream.DecodeNumber(MD[Audio.CurChannel]);
        if (256 == num)
        {
          ReadTables();
        }
      }
      else
      {
        const uint_t num = Stream.DecodeNumber(LD);
        if (269 == num)
        {
          ReadTables();
        }
      }
    }

    template<uint_t treeSize>
    void MakeDecodeTable(const uint_t* lenTab, DecodeTree<treeSize>& tree)
    {
      boost::array<uint_t, 16> lenCount = { {0} };

      for (uint_t i = 0; i < treeSize; ++i)
      {
        ++lenCount[lenTab[i] & 0x0f];
      }
      lenCount[0] = 0;

      FillTreeDecodeLen(&lenCount[0], tree);
      boost::array<uint_t, 16> tmpPos = tree.DecodePos;

      for (uint_t i = 0; i < treeSize; ++i)
      {
        if (lenTab[i])
        {
          tree.DecodeNum[tmpPos[lenTab[i] & 0xf]++] = i;
        }
      }
    }

    void FillTreeDecodeLen(const uint_t* lenCount, BaseDecodeTree& tree)
    {
      tree.DecodePos[0] = tree.DecodeLen[0] = 0;
      for (uint_t n = 0, i = 1; i < 16; ++i)
      {
        n = 2 * (n + lenCount[i]);
        tree.DecodeLen[i] = std::min<uint_t>(n << (15 - i), 0xffff);
        tree.DecodePos[i] = tree.DecodePos[i - 1] + lenCount[i - 1];
      }
    }

    void DecodeAudioBlock()
    {
      while (!Decoded.Full())
      {
        const uint_t num = Stream.DecodeNumber(MD[Audio.CurChannel]);
        if (256 == num)
        {
          ReadTables();
          break;
        }
        Decoded.Add(DecodeAudio(num));
        Audio.NextChannel();
      }
    }

    void DecodeDataBlock()
    {
      static const uint_t LDecode[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224}; 
      static const uint_t LBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
      static const uint_t DDecode[] = 
      {
        0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576,
        32768, 49152, 65536, 98304, 131072, 196608, 262144, 327680, 393216, 458752, 524288, 589824, 655360, 720896, 786432, 851968, 917504, 983040
      }; 
      static const uint_t DBits[] =
      {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
      };
      static const uint_t SDDecode[] = {0, 4, 8, 16, 32, 64, 128, 192};
      static const uint_t SDBits[] = {2, 2, 3, 4, 5, 6, 6, 6};

      while (!Decoded.Full())
      {
        const uint_t num = Stream.DecodeNumber(LD);
        if (num < 256)
        {
          Decoded.Add(static_cast<uint8_t>(num));
        }
        else if (num > 269)
        {
          const uint_t lenIdx = num - 270;
          uint_t len = LDecode[lenIdx] + 3;
          if (const uint_t lenBits = LBits[lenIdx])
          {
            len += Stream.ReadBits(lenBits);
          }
          const uint_t distIdx = Stream.DecodeNumber(DD);
          uint_t dist = DDecode[distIdx] + 1;
          if (const uint_t distBits = DBits[distIdx])
          {
            dist += Stream.ReadBits(distBits);
          }
          if (dist >= 0x40000L)
          {
            ++len;
          }
          if (dist >= 0x2000L)
          {
            ++len;
          }
          CopyString(dist, len);
        }
        else if (num == 269)
        {
          ReadTables();
          break;
        }
        else if (num == 256)
        {
          CopyString(History.LastDist, History.LastCount);
        }
        else if (num < 261)
        {
          const uint_t dist = History.GetDist(num - 256);
          const uint_t lenIdx = Stream.DecodeNumber(RD);
          uint_t len = LDecode[lenIdx] + 2;
          if (const uint_t lenBits = LBits[lenIdx])
          {
            len += Stream.ReadBits(lenBits);
          }
          if (dist >= 0x40000L)
          {
            ++len;
          }
          if (dist >= 0x2000)
          {
            ++len;
          }
          if (dist >= 0x101)
          {
            ++len;
          }
          CopyString(dist, len);
        }
        else if (num < 270)
        {
          const uint_t distIdx = num - 261;
          uint_t dist = SDDecode[distIdx] + 1;
          if (const uint_t distBits = SDBits[distIdx])
          {
            dist += Stream.ReadBits(distBits);
          }
          CopyString(dist, 2);
        }
      }
    }

    uint8_t DecodeAudio(int_t delta)
    {
      MultimediaCompression::VarType& var = Audio.Variables[Audio.CurChannel];
      ++var.ByteCount;
      var.D[3] = var.D[2];
      var.D[2] = var.D[1];
      var.D[1] = var.LastDelta - var.D[0];
      var.D[0] = var.LastDelta;
      const int predicted = 
        ((var.LastChar * 8 + 
          var.K[0] * var.D[0] + 
          var.K[1] * var.D[1] + 
          var.K[2] * var.D[2] + 
          var.K[3] * var.D[3] +
          var.K[4] * Audio.ChannelDelta) >> 3) & 0xff;
      const uint_t val = predicted - delta;
      const int_t idx = static_cast<int8_t>(delta) << 3;
      var.Dif[0] += absolute(idx);
      var.Dif[1] += absolute(idx - var.D[0]);
      var.Dif[2] += absolute(idx + var.D[0]);
      var.Dif[3] += absolute(idx - var.D[1]);
      var.Dif[4] += absolute(idx + var.D[1]);
      var.Dif[5] += absolute(idx - var.D[2]);
      var.Dif[6] += absolute(idx + var.D[2]);
      var.Dif[7] += absolute(idx - var.D[3]);
      var.Dif[8] += absolute(idx + var.D[3]);
      var.Dif[9] += absolute(idx - Audio.ChannelDelta);
      var.Dif[10] += absolute(idx + Audio.ChannelDelta);

      Audio.ChannelDelta = var.LastDelta = static_cast<int8_t>(val - var.LastChar);
      var.LastChar = val;
      if (var.ByteCount & 0x1f)
      {
        return static_cast<uint8_t>(val);
      }
      const boost::array<uint_t, 11>::const_iterator minDifIt = std::min_element(var.Dif.begin(), var.Dif.end());
      var.Dif.assign(0);
      if (const std::size_t minIdx = minDifIt - var.Dif.begin())
      {
        int_t& k = var.K[(minIdx - 1) / 2];
        if (minIdx & 1)
        {
          if (k >= -16)
          {
            --k;
          }
        }
        else
        {
          if (k < 16)
          {
            ++k;
          }
        }
      }
      return static_cast<uint8_t>(val);
    }

    void CopyString(uint_t dist, uint_t count)
    {
      if (!Decoded.CopyFromBack(dist, count))
      {
        Log::Debug(THIS_MODULE, "Invalid backreference!");
      }
      History.Store(dist, count);
    }
  private:
    RarBitstream Stream;
  
    struct MultimediaCompression
    {
      struct VarType
      {
        boost::array<int_t, 5> K;
        boost::array<int_t, 4> D;
        int_t LastDelta;
        boost::array<uint_t, 11> Dif;
        uint_t ByteCount;
        int_t LastChar;

        VarType()
          : K()
          , D()
          , LastDelta()
          , Dif()
          , ByteCount()
          , LastChar()
        {
        }
      };

      bool Unpack;
      uint_t Channels;
      uint_t CurChannel;
      int_t ChannelDelta;
      boost::array<VarType, 4> Variables;

      MultimediaCompression()
        : Unpack()
        , Channels()
        , CurChannel()
        , ChannelDelta()
        , Variables()
      {
      }

      void SetChannels(uint_t count)
      {
        Channels = count;
        if (CurChannel >= Channels)
        {
          CurChannel = 0;
        }
      }

      void NextChannel()
      {
        if (++CurChannel >= Channels)
        {
          CurChannel = 0;
        }
      }
    } Audio;
    struct RefHistory
    {
      uint_t LastDist;
      uint_t LastCount;
      uint_t OldDistances[4];
      uint_t CurHistory;

      RefHistory()
        : LastDist()
        , LastCount()
        , CurHistory()
      {
      }

      void Store(uint_t dist, uint_t count)
      {
        LastDist = dist;
        LastCount = count;
        OldDistances[CurHistory++ & 3] = dist;
      }

      uint_t GetDist(uint_t depth) const
      {
        return OldDistances[(CurHistory - depth) & 3];
      }
    } History;
    boost::array<uint_t, MC * 4> UnpOldTable;
    RepDecodeTree RD;
    BitDecodeTree BD;
    LitDecodeTree LD;
    DistDecodeTree DD;
    boost::array<MultDecodeTree, 4> MD;
    DecodeTarget Decoded;
  };

  class PackedFile : public CompressedFile
  {
  public:
    PackedFile(const uint8_t* const start, std::size_t size, std::size_t destSize)
      : Start(start)
      , Size(size)
      , DestSize(destSize)
    {
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      Log::Debug(THIS_MODULE, "Depack %1% -> %2%", Size, DestSize);
      const std::clock_t startTick = std::clock();
      RarDecoder decoder(Start, Size, DestSize);
      if (Dump* decoded = decoder.GetDecodedData())
      {
        const uint_t timeSpent = std::clock() - startTick;
        const uint_t speed = timeSpent ? (decoded->size() * CLOCKS_PER_SEC / timeSpent) : 0;
        Log::Debug(THIS_MODULE, "Depacking speed is %1% b/s", speed);
        std::auto_ptr<Dump> res(new Dump());
        res->swap(*decoded);
        return res;
      }
      return std::auto_ptr<Dump>();
    }
  private:
    const uint8_t* const Start;
    const std::size_t Size;
    const std::size_t DestSize;
  };

  std::auto_ptr<CompressedFile> CreateDecoder(const Formats::Packed::Rar::FileBlockHeader& header)
  {
    const uint8_t* const start = safe_ptr_cast<const uint8_t*>(&header) + fromLE(header.Size);
    const std::size_t size = fromLE(header.AdditionalSize);
    const std::size_t outSize = fromLE(header.UnpackedSize);
    switch (header.Method)
    {
    case 0x30:
      return std::auto_ptr<CompressedFile>(new NoCompressedFile(start, size, outSize));
    default:
      return std::auto_ptr<CompressedFile>(new PackedFile(start, size, outSize));
    }
    return std::auto_ptr<CompressedFile>();
  }
 
  class DispatchedCompressedFile : public CompressedFile
  {
  public:
    explicit DispatchedCompressedFile(const Container& container)
      : Header(container.GetHeader())
      , Delegate(CreateDecoder(Header))
      , IsValid(container.FastCheck() && Delegate.get())
    {
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      if (!IsValid)
      {
        return std::auto_ptr<Dump>();
      }
      std::auto_ptr<Dump> result = Delegate->Decompress();
      IsValid = result.get() && result->size() == fromLE(Header.UnpackedSize);
      return result;
    }
  private:
    const Formats::Packed::Rar::FileBlockHeader& Header;
    const std::auto_ptr<CompressedFile> Delegate;
    mutable bool IsValid;
  };
}

namespace Formats
{
  namespace Packed
  {
    namespace Rar
    {
      String FileBlockHeader::GetName() const
      {
        const uint8_t* const self = safe_ptr_cast<const uint8_t*>(this);
        const uint8_t* const filename = self + (IsBigFile() ? sizeof(BigFileBlockHeader) : sizeof(FileBlockHeader));
        return String(filename, filename + fromLE(NameSize));
      }

      bool FileBlockHeader::IsValid() const
      {
        return IsExtended() && Type == TYPE;
      }

      bool FileBlockHeader::IsSupported() const
      {
        const uint_t flags = fromLE(Flags);
        //multivolume files are not suported
        if (0 != (flags & (FileBlockHeader::FLAG_SPLIT_BEFORE | FileBlockHeader::FLAG_SPLIT_AFTER)))
        {
          return false;
        }
        //encrypted files are not supported
        if (0 != (flags & FileBlockHeader::FLAG_ENCRYPTED))
        {
          return false;
        }
        //solid files are not supported
        if (0 != (flags & FileBlockHeader::FLAG_SOLID))
        {
          return false;
        }
        //big files are not supported
        if (IsBigFile())
        {
          return false;
        }
        //skip directory
        if (FileBlockHeader::FLAG_DIRECTORY == (flags & FileBlockHeader::FLAG_DIRECTORY))
        {
          return false;
        }
        //skip empty files
        if (0 == UnpackedSize)
        {
          return false;
        }
        //skip invalid version
        if (!in_range<uint_t>(DepackerVersion, MIN_VERSION, MAX_VERSION))
        {
          return false;
        }
        return true;
      }
    }

    class RarDecoder : public Decoder
    {
    public:
      RarDecoder()
        : Depacker(DataFormat::Create(::Rar::HEADER_PATTERN))
      {
      }

      virtual DataFormat::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const ::Rar::Container container(data, availSize);
        return container.FastCheck();
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const ::Rar::Container container(data, availSize);
        if (!container.FastCheck())
        {
          return std::auto_ptr<Dump>();
        }
        ::Rar::DispatchedCompressedFile decoder(container);
        std::auto_ptr<Dump> decoded = decoder.Decompress();
        if (decoded.get())
        {
          usedSize = container.GetUsedSize();
        }
        return decoded;
      }
    private:
      const DataFormat::Ptr Depacker;
    };

    Decoder::Ptr CreateRarDecoder()
    {
      return boost::make_shared<RarDecoder>();
    }
  }
}
