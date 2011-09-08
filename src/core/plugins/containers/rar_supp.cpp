/*
Abstract:
  Rar convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "core/plugins/registrator.h"
#include "core/src/path.h"
#include "formats/packed/pack_utils.h"
//common includes
#include <byteorder.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <formats/packed_decoders.h>
//std includes
#include <numeric>
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'R', 'A', 'R', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::RAR_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK;

  const std::string THIS_MODULE("Core::RARSupp");

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct BlockHeader
  {
    uint16_t CRC;
    uint8_t Type;
    uint16_t Flags;
    uint16_t Size;

    enum
    {
      FLAG_HAS_TAIL = 0x8000,

      MIN_TYPE = 0x72,
      MAX_TYPE = 0x7b
    };

    bool IsExtended() const
    {
      return 0 != (fromLE(Flags) & FLAG_HAS_TAIL);
    }
  } PACK_POST;

  const BlockHeader MARKER = 
  {
    fromLE(0x6152),
    0x72,
    fromLE(0x1a21),
    fromLE(0x0007)
  };

  PACK_PRE struct ExtendedBlockHeader : BlockHeader
  {
    uint32_t AdditionalSize;
  } PACK_POST;

  // ArchiveBlockHeader is always BlockHeader
  PACK_PRE struct ArchiveBlockHeader : BlockHeader
  {
    static const uint8_t TYPE = 0x73;

    //CRC from Type till Reserved2
    uint16_t Reserved1;
    uint32_t Reserved2;

    enum
    {
      FLAG_VOLUME = 1,
      FLAG_HAS_COMMENT = 2,
      FLAG_BLOCKED = 4,
      FLAG_SOLID = 8,
      FLAG_SIGNATURE = 0x20,
    };
  } PACK_POST;

  // File header is always ExtendedBlockHeader
  PACK_PRE struct FileBlockHeader : ExtendedBlockHeader
  {
    static const uint8_t TYPE = 0x74;

    //CRC from Type to Attributes+
    uint32_t UnpackedSize;
    uint8_t HostOS;
    uint32_t UnpackedCRC;
    uint32_t TimeStamp;
    uint8_t DepackerVersion;
    uint8_t Method;
    uint16_t NameSize;
    uint32_t Attributes;

    enum
    {
      FLAG_SPLIT_BEFORE = 1,
      FLAG_SPLIT_AFTER = 2,
      FLAG_ENCRYPTED = 4,
      FLAG_HAS_COMMENT = 8,
      FLAG_SOLID = 0x10,
      FLAG_DIRECTORY = 0xe0,
      FLAG_BIG_FILE = 0x100,

      MIN_VERSION = 13,
      MAX_VERSION = 20
    };

    bool IsBigFile() const
    {
      return 0 != (fromLE(Flags) & FLAG_BIG_FILE);
    }

    String GetName() const;
  } PACK_POST;

  PACK_PRE struct BigFileBlockHeader : FileBlockHeader
  {
    uint32_t PackedSizeHi;
    uint32_t UnpackedSizeHi;
  } PACK_POST; 

  inline String FileBlockHeader::GetName() const
  {
    const uint8_t* const self = safe_ptr_cast<const uint8_t*>(this);
    const uint8_t* const filename = self + (IsBigFile() ? sizeof(BigFileBlockHeader) : sizeof(FileBlockHeader));
    return String(filename, filename + fromLE(NameSize));
  }

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

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
    boost::array<uint_t, 16>  DecodePos;

    BaseDecodeTree()
      : DecodeLen()
      , DecodePos()
    {
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
      , LRange()
      , LCode()
    {
      FetchBits(0);
    }

    uint32_t ReadBits(uint_t bits)
    {
      const uint32_t result = PeekBits(bits);
      FetchBits(bits);
      return result;
    }

    uint32_t PeekBits(uint_t bits)
    {
      static const boost::array<uint32_t, 16> BitMask =
      {
        {
         1, 3, 7, 15, 31, 63, 127, 255,
         511, 1023, 2047, 4095, 8191, 16383, 32767, 65535
        }
      };
      assert(LRange >= bits);
      return BitMask[bits - 1] & (LCode >> (LRange - bits));
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
    void FetchBits(uint_t bits)
    {
      LRange -= bits;
      while (LRange < 24)
      {
        LCode = (LCode << 8) | GetByte();
        LRange += 8;
      }
    }

    uint_t DecodeNumIndex(const BaseDecodeTree& tree)
    {
      const uint_t len = PeekBits(16) & 0xfffe;
      uint_t bits = 1;
      while (bits < 15 && len >= tree.DecodeLen[bits])
      {
        ++bits;
      }
      FetchBits(bits);
      return tree.DecodePos[bits] + ((len - tree.DecodeLen[bits - 1]) >> (16 - bits));
    }
  private:
    uint32_t LRange;
    uint32_t LCode;
  };

  class RarDecoder
  {
  public:
    RarDecoder(const uint8_t* const start, std::size_t size, std::size_t destSize)
      : Stream(start, size)
      , TargetSize(destSize)
    {
    }

    Dump* GetDecodedData()
    {
      if (Decoded.empty())
      {
        Decode();
      }
      return !Decoded.empty()
        ? &Decoded
        : 0;
    }
  private:
    void Decode()
    {
      static const uint_t LDecode[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224}; 
      static const uint_t LBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
      static const uint_t DDecode[] = 
      {
        0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768,
        49152, 65536, 98304, 131072, 196608, 262144, 327680, 393216, 458752, 524288, 589824, 655360, 720896, 786432, 851968, 917504, 983040
      }; 
      static const uint_t DBits[] =
      {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
      };
      static const uint_t SDDecode[] = {0, 4, 8, 16, 32, 64, 128, 192};
      static const uint_t SDBits[] = {2, 2, 3, 4, 5, 6, 6, 6};

      const std::size_t prevSize = Decoded.size();
      const std::size_t requiredSize = prevSize + TargetSize;
      Decoded.reserve(requiredSize);
      ReadTables();
      while (Decoded.size() < requiredSize)
      {
        if (Audio.Unpack)
        {
          const uint_t num = Stream.DecodeNumber(MD[Audio.CurChannel]);
          if (256 == num)
          {
            ReadTables();
            continue;
          }
          Decoded.push_back(DecodeAudio(num));
          Audio.NextChannel();
          continue;
        }
        const uint_t num = Stream.DecodeNumber(LD);
        if (num < 256)
        {
          Decoded.push_back(static_cast<uint8_t>(num));
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

    void CopyString(uint_t dist, uint_t count)
    {
      CopyFromBack(dist, Decoded, count);
      History.Store(dist, count);
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
  private:
    RarBitstream Stream;
    const std::size_t TargetSize;
  
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
    Dump Decoded;
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

  class RaredFile : public CompressedFile
  {
  public:
    explicit RaredFile(const FileBlockHeader& header)
    {
      const uint8_t* const start = safe_ptr_cast<const uint8_t*>(&header) + fromLE(header.Size);
      const std::size_t size = fromLE(header.AdditionalSize);
      const std::size_t outSize = fromLE(header.UnpackedSize);
      switch (header.Method)
      {
      case 0x30:
        Delegate.reset(new NoCompressedFile(start, size, outSize));
        break;
      default:
        Delegate.reset(new PackedFile(start, size, outSize));
        break;
      }
    }

    virtual std::auto_ptr<Dump> Decompress() const
    {
      return Delegate.get()
        ? Delegate->Decompress()
        : std::auto_ptr<Dump>();
    }
  private:
    std::auto_ptr<CompressedFile> Delegate;
  };

  class RarContainerFile : public Container::File
  {
  public:
    RarContainerFile(const String& name, IO::DataContainer::Ptr data)
      : Name(name)
      , Data(data)
      , Header(*static_cast<const FileBlockHeader*>(Data->Data()))
    {
      assert(!Header.IsBigFile());
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetOffset() const
    {
      return ~std::size_t(0);
    }

    virtual std::size_t GetSize() const
    {
      return fromLE(Header.UnpackedSize);
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      Log::Debug(THIS_MODULE, "Decompressing '%1%'", GetName());
      const CompressedFile::Ptr file(new RaredFile(Header));
      std::auto_ptr<Dump> decoded = file->Decompress();
      if (!decoded.get())
      {
        return IO::DataContainer::Ptr();
      }
      return IO::CreateDataContainer(decoded);
    }
  private:
    const String Name;
    const IO::DataContainer::Ptr Data;
    const FileBlockHeader& Header;
  };

  class RarBlocksIterator
  {
  public:
    explicit RarBlocksIterator(IO::DataContainer::Ptr data)
      : Data(data)
      , Limit(Data->Size())
      , Offset(0)
    {
      assert(0 == std::memcmp(Get<BlockHeader>(), &MARKER, sizeof(MARKER)));
    }

    bool IsEof() const
    {
      const uint64_t curBlockSize = GetBlockSize();
      return !curBlockSize || uint64_t(Offset) + curBlockSize > uint64_t(Limit);
    }

    const BlockHeader& GetCurrentBlock() const
    {
      return *Get<BlockHeader>();
    }

    const ArchiveBlockHeader* GetArchiveHeader() const
    {
      assert(!IsEof());
      const ArchiveBlockHeader& block = *Get<ArchiveBlockHeader>();
      return !block.IsExtended() && ArchiveBlockHeader::TYPE == block.Type
        ? &block
        : 0;
    }

    const FileBlockHeader* GetFileHeader() const
    {
      assert(!IsEof());
      if (const FileBlockHeader* const block = Get<FileBlockHeader>())
      {
        return block->IsExtended() && FileBlockHeader::TYPE == block->Type
          ? block
          : 0;
      }
      return 0;
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }

    uint64_t GetBlockSize() const
    {
      if (const BlockHeader* const block = Get<BlockHeader>())
      {
        uint64_t res = fromLE(block->Size);
        if (block->IsExtended())
        {
          if (const ExtendedBlockHeader* const extBlock = Get<ExtendedBlockHeader>())
          {
            res += fromLE(extBlock->AdditionalSize);
            //Even if big files are not supported, we should properly skip them in stream
            if (FileBlockHeader::TYPE == extBlock->Type && 
                FileBlockHeader::FLAG_BIG_FILE & fromLE(extBlock->Flags))
            {
              if (const BigFileBlockHeader* const bigFile = Get<BigFileBlockHeader>())
              {
                res += uint64_t(fromLE(bigFile->PackedSizeHi)) << (8 * sizeof(uint32_t));
              }
              else
              {
                return sizeof(*bigFile);
              }
            }
          }
          else
          {
            return sizeof(*extBlock);
          }
        }
        return res;
      }
      else
      {
        return sizeof(*block);
      }
    }

    void Next()
    {
      assert(!IsEof());
      Offset += GetBlockSize();
      if (const BlockHeader* block = Get<BlockHeader>())
      {
        //finish block
        if (block->Type == 0x7b && 7 == fromLE(block->Size))
        {
          Offset += sizeof(*block);
        }
      }
    }
  private:
    template<class T>
    const T* Get() const
    {
      if (Limit - Offset >= sizeof(T))
      {
        return safe_ptr_cast<const T*>(static_cast<const uint8_t*>(Data->Data()) + Offset);
      }
      else
      {
        return 0;
      }
    }
  private:
    const IO::DataContainer::Ptr Data;
    const std::size_t Limit;
    std::size_t Offset;
  };

  class RarFilesIterator
  {
  public:
    explicit RarFilesIterator(IO::DataContainer::Ptr data)
      : Data(data)
      , Blocks(data)
    {
      SkipNonFileBlocks();
    }

    bool IsEof() const
    {
      return Blocks.IsEof();
    }

    bool IsValid() const
    {
      assert(!IsEof());
      const FileBlockHeader& file = *Blocks.GetFileHeader();
      const uint_t flags = fromLE(file.Flags);
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
      if (file.IsBigFile())
      {
        return false;
      }
      //skip directory
      if (FileBlockHeader::FLAG_DIRECTORY == (flags & FileBlockHeader::FLAG_DIRECTORY))
      {
        return false;
      }
      //skip empty files
      if (0 == file.UnpackedSize)
      {
        return false;
      }
      //skip invalid version
      if (!in_range<uint_t>(file.DepackerVersion, FileBlockHeader::MIN_VERSION, FileBlockHeader::MAX_VERSION))
      {
        return false;
      }
      return true;
    }

    String GetName() const
    {
      const FileBlockHeader& file = *Blocks.GetFileHeader();
      String name = file.GetName();
      std::replace(name.begin(), name.end(), '\\', '/');
      return name;
    }

    Container::File::Ptr GetFile() const
    {
      assert(IsValid());
      const std::size_t offset = Blocks.GetOffset();
      const std::size_t size = Blocks.GetBlockSize();
      const IO::DataContainer::Ptr fileBlock = Data->GetSubcontainer(offset, size);
      return boost::make_shared<RarContainerFile>(GetName(), fileBlock);
    }

    void Next()
    {
      assert(!IsEof());
      Blocks.Next();
      SkipNonFileBlocks();
    }

    std::size_t GetOffset() const
    {
      return Blocks.GetOffset();
    }
  private:
    void SkipNonFileBlocks()
    {
      while (!Blocks.IsEof() && !Blocks.GetFileHeader())
      {
        Blocks.Next();
      }
    }
  private:
    const IO::DataContainer::Ptr Data;
    RarBlocksIterator Blocks;
  };

  template<class It>
  class FilesIterator : public Container::Catalogue::Iterator
  {
  public:
    FilesIterator(It begin, It limit)
      : Current(begin)
      , Limit(limit)
    {
    }

    virtual bool IsValid() const
    {
      return Current != Limit;
    }

    virtual Container::File::Ptr Get() const
    {
      assert(IsValid());
      return Current->second;
    }

    virtual void Next()
    {
      assert(IsValid());
      ++Current;
    }
  private:
    It Current;
    const It Limit;
  };

  class RarCatalogue : public Container::Catalogue
  {
  public:
    RarCatalogue(std::size_t maxFileSize, IO::DataContainer::Ptr data)
      : Data(data)
      , MaxFileSize(maxFileSize)
    {
    }

    virtual Iterator::Ptr GetFiles() const
    {
      FillCache();
      return Iterator::Ptr(new FilesIterator<FilesMap::const_iterator>(Files.begin(), Files.end()));
    }

    virtual uint_t GetFilesCount() const
    {
      FillCache();
      return Files.size();
    }

    virtual Container::File::Ptr FindFile(const DataPath& path) const
    {
      const String inPath = path.AsString();
      const String firstComponent = path.GetFirstComponent();
      if (inPath == firstComponent)
      {
        return FindFile(firstComponent);
      }
      Log::Debug(THIS_MODULE, "Opening '%1%'", inPath);
      DataPath::Ptr resolved = CreateDataPath(firstComponent);
      for (;;)
      {
        const String filename = resolved->AsString();
        Log::Debug(THIS_MODULE, "Trying '%1%'", filename);
        if (Container::File::Ptr file = FindFile(filename))
        {
          Log::Debug(THIS_MODULE, "Found");
          return file;
        }
        if (filename == inPath)
        {
          return Container::File::Ptr();
        }
        const DataPath::Ptr unresolved = SubstractDataPath(path, *resolved);
        resolved = CreateMergedDataPath(resolved, unresolved->GetFirstComponent());
      }
    }

    virtual std::size_t GetSize() const
    {
      FillCache();
      assert(Iter->IsEof());
      return Iter->GetOffset();
    }
  private:
    void FillCache() const
    {
      FindNonCachedFile(String());
    }

    Container::File::Ptr FindFile(const String& name) const
    {
      if (Container::File::Ptr file = FindCachedFile(name))
      {
        return file;
      }
      return FindNonCachedFile(name);
    }

    Container::File::Ptr FindCachedFile(const String& name) const
    {
      if (Iter.get())
      {
        const FilesMap::const_iterator it = Files.find(name);
        if (it != Files.end())
        {
          return it->second;
        }
      }
      return Container::File::Ptr();
    }

    Container::File::Ptr FindNonCachedFile(const String& name) const
    {
      CreateIterator();
      while (!Iter->IsEof())
      {
        const String fileName = Iter->GetName();
        if (!Iter->IsValid())
        {
          Log::Debug(THIS_MODULE, "Invalid file '%1%'", fileName);
          Iter->Next();
          continue;
        }
        Log::Debug(THIS_MODULE, "Found file '%1%'", fileName);
        const Container::File::Ptr fileObject = Iter->GetFile();
        Iter->Next();
        if (fileObject->GetSize() > MaxFileSize)
        {
          Log::Debug(THIS_MODULE, "File is too big (%1% bytes)", fileObject->GetSize());
          continue;
        }
        Files.insert(FilesMap::value_type(fileName, fileObject));
        if (fileName == name)
        {
          return fileObject;
        }
      }
      return Container::File::Ptr();
    }

    void CreateIterator() const
    {
      if (!Iter.get())
      {
        Iter.reset(new RarFilesIterator(Data));
      }
    }
  private:
    const IO::DataContainer::Ptr Data;
    const std::size_t MaxFileSize;
    mutable std::auto_ptr<RarFilesIterator> Iter;
    typedef std::map<String, Container::File::Ptr> FilesMap;
    mutable FilesMap Files;
  };

  const std::string RAR_FORMAT(
    //file marker
    "5261"       //uint16_t CRC;   "Ra"
    "72"         //uint8_t Type;   "r"
    "211a"       //uint16_t Flags; "!ESC^"
    "0700"       //uint16_t Size;  "BELL^,0"
    //archive header
    "??"         //uint16_t CRC;
    "73"         //uint8_t Type;
    "%xxxx0xxx?" //uint16_t Flags; skip Solid archives
    "0d00"       //uint16_t Size
  );

  class RarFactory : public ContainerFactory
  {
  public:
    RarFactory()
      : Format(DataFormat::Create(RAR_FORMAT))
    {
    }

    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Container::Catalogue::Ptr CreateContainer(const Parameters::Accessor& parameters, IO::DataContainer::Ptr data) const
    {
      if (!Format->Match(data->Data(), data->Size()))
      {
        return Container::Catalogue::Ptr();
      }
      //TODO: own parameters
      Parameters::IntType maxFileSize = Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB_DEFAULT;
      parameters.FindIntValue(Parameters::ZXTune::Core::Plugins::ZIP::MAX_DEPACKED_FILE_SIZE_MB, maxFileSize);
      maxFileSize *= 1 << 20;
      if (maxFileSize > std::numeric_limits<std::size_t>::max())
      {
        maxFileSize = std::numeric_limits<std::size_t>::max();
      }
      return boost::make_shared<RarCatalogue>(static_cast<std::size_t>(maxFileSize), data);
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterRarContainer(PluginsRegistrator& registrator)
  {
    const ContainerFactory::Ptr factory = boost::make_shared<RarFactory>();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
