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
#include "container.h"
#include "rar_supp.h"
#include "pack_utils.h"
//common includes
#include <debug_log.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
#include <formats/packed.h>
#include <math/numeric.h>
//std includes
#include <cassert>
#include <memory>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/packed.h>

namespace RarFile
{
  const Debug::Stream Dbg("Formats::Packed::Rar");

  const std::string HEADER_PATTERN =
    "??"          // uint16_t CRC;
    "74"          // uint8_t Type;
    "?%1xxxxxxx"  // uint16_t Flags;
  ;

  class Container : public Binary::TypedContainer
  {
  public:
    explicit Container(const Binary::Container& data)
      : Binary::TypedContainer(data)
      , Data(data)
    {
    }

    bool FastCheck() const
    {
      if (std::size_t usedSize = GetUsedSize())
      {
        return usedSize <= Data.Size();
      }
      return false;
    }

    std::size_t GetUsedSize() const
    {
      if (const Formats::Packed::Rar::FileBlockHeader* header = GetField<Formats::Packed::Rar::FileBlockHeader>(0))
      {
        if (!header->IsValid() || !header->IsSupported())
        {
          return 0;
        }
        uint64_t res = fromLE(header->Size);
        res += fromLE(header->AdditionalSize);
        if (header->IsBigFile())
        {
          if (const Formats::Packed::Rar::BigFileBlockHeader* bigHeader = GetField<Formats::Packed::Rar::BigFileBlockHeader>(0))
          {
            res += uint64_t(fromLE(bigHeader->PackedSizeHi)) << (8 * sizeof(uint32_t));
          }
          else
          {
            return 0;
          }
        }
        const std::size_t maximum = std::numeric_limits<std::size_t>::max();
        return res > maximum
          ? maximum
          : static_cast<std::size_t>(res);
      }
      return false;
    }

    const Formats::Packed::Rar::FileBlockHeader& GetHeader() const
    {
      return *GetField<Formats::Packed::Rar::FileBlockHeader>(0);
    }

    const Binary::Container& GetData() const
    {
      return Data;
    }
  private:
    const Binary::Container& Data;
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

  const std::size_t MAX_WINDOW_SIZE = 0x400000;

  class DecodeTarget
  {
  public:
    DecodeTarget()
      : Rest(0)
    {
    }

    Binary::Container::Ptr Reallocate(std::size_t size)
    {
      if (Data)
      {
        const std::size_t prevSize = Data->size();
        const std::size_t toCopy = std::min(MAX_WINDOW_SIZE, prevSize);
        const boost::shared_ptr<Dump> newData = boost::make_shared<Dump>(toCopy + size);
        const std::size_t toSkip = prevSize - toCopy;
        Cursor = std::copy(Data->begin() + toSkip, Data->end(), newData->begin());
        Data = newData;
        Rest = size;
        Dbg("Reallocate memory %1% -> %2% (%3% in context)", prevSize, size, toCopy);
        return Binary::CreateContainer(Data, toCopy, size);
      }
      else
      {
        return Allocate(size);
      }
    }

    Binary::Container::Ptr Allocate(std::size_t size)
    {
      Data = boost::make_shared<Dump>(size);
      Cursor = Data->begin();
      Rest = size;
      Dbg("Allocated memory %1%", size);
      return Binary::CreateContainer(Data, 0, size);
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

    bool CopyFromBack(uint_t offset, std::size_t count)
    {
      if (offset > uint_t(Cursor - Data->begin()))
      {
        return false;
      }
      const std::size_t realSize = std::min(count, Rest);
      const Dump::const_iterator srcStart = Cursor - offset;
      const Dump::const_iterator srcEnd = srcStart + realSize;
      RecursiveCopy(srcStart, srcEnd, Cursor);
      Cursor += realSize;
      Rest -= realSize;
      return true;
    }
  private:
    boost::shared_ptr<Dump> Data;
    //std::size_t Done;
    Dump::iterator Cursor;
    std::size_t Rest;
  };

  class SpeedMeasure
  {
  public:
    explicit SpeedMeasure(std::size_t targetSize)
      : TargetSize(targetSize)
      , StartTick(std::clock())
    {
    }

    ~SpeedMeasure()
    {
      const uint_t timeSpent = std::clock() - StartTick;
      if (timeSpent)
      {
        const std::size_t speed = TargetSize * CLOCKS_PER_SEC / timeSpent;
        Dbg("Depacking speed is %1% b/s", speed);
      }
      else
      {
        Dbg("Unable to determine depacking speed.");
      }
    }
  private:
    const std::size_t TargetSize;
    const std::clock_t StartTick;
  };

  class RarDecoder
  {
  public:
    Binary::Container::Ptr Decode(RarBitstream& source, std::size_t destSize, bool solid)
    {
      if (!solid)
      {
        Reset();
        ReadTables(source);
      }
      const Binary::Container::Ptr res = solid ? Decoded.Reallocate(destSize) : Decoded.Allocate(destSize);
      {
        const SpeedMeasure measure(destSize);
        Decode(source);
      }
      return res;
    }
  private:
    void Reset()
    {
      Audio = MultimediaCompression();
      History = RefHistory();
      UnpOldTable.assign(0);
    }

    void ReadTables(RarBitstream& stream)
    {
      const uint_t flags = stream.ReadBits(2);
      Audio.Unpack = 0 != (flags & 2);
      if (0 == (flags & 1))
      {
        UnpOldTable.assign(0);
      }

      if (Audio.Unpack)
      {
        const uint_t channels = stream.ReadBits(2) + 1;
        Audio.SetChannels(channels);
      }
      const uint_t tableSize = Audio.Unpack
        ? MC * Audio.Channels
        : NC + DC + RC;

      boost::array<uint_t, BC> bitCount =  { {0} };
      for (uint_t i = 0; i < bitCount.size(); ++i)
      {
        bitCount[i] = stream.ReadBits(4);
      }
      MakeDecodeTable(&bitCount[0], BD);
      boost::array<uint_t, MC * 4> table = { {0} };
      assert(tableSize <= table.size());
      for (uint_t i = 0; i < tableSize; )
      {
        const uint_t number = stream.DecodeNumber(BD);
        if (number < 16)
        {
          table[i] = (number + UnpOldTable[i]) & 0xf;
          ++i;
        }
        else if (number == 16)
        {
          uint_t n = stream.ReadBits(2) + 3;
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
            ? (stream.ReadBits(3) + 3)
            : (stream.ReadBits(7) + 11);
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

    void Decode(RarBitstream& source)
    {
      while (!Decoded.Full())
      {
        if (Audio.Unpack)
        {
          DecodeAudioBlock(source);
        }
        else
        {
          DecodeDataBlock(source);
        }
      }
      ReadLastTables(source);
    }

    void ReadLastTables(RarBitstream& stream)
    {
      if (Audio.Unpack)
      {
        const uint_t num = stream.DecodeNumber(MD[Audio.CurChannel]);
        if (256 == num)
        {
          ReadTables(stream);
        }
      }
      else
      {
        const uint_t num = stream.DecodeNumber(LD);
        if (269 == num)
        {
          ReadTables(stream);
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

    void DecodeAudioBlock(RarBitstream& source)
    {
      while (!Decoded.Full())
      {
        const uint_t num = source.DecodeNumber(MD[Audio.CurChannel]);
        if (256 == num)
        {
          ReadTables(source);
          break;
        }
        Decoded.Add(DecodeAudio(num));
        Audio.NextChannel();
      }
    }

    void DecodeDataBlock(RarBitstream& source)
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
        const uint_t num = source.DecodeNumber(LD);
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
            len += source.ReadBits(lenBits);
          }
          const uint_t distIdx = source.DecodeNumber(DD);
          uint_t dist = DDecode[distIdx] + 1;
          if (const uint_t distBits = DBits[distIdx])
          {
            dist += source.ReadBits(distBits);
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
          ReadTables(source);
          break;
        }
        else if (num == 256)
        {
          CopyString(History.LastDist, History.LastCount);
        }
        else if (num < 261)
        {
          const uint_t dist = History.GetDist(num - 256);
          const uint_t lenIdx = source.DecodeNumber(RD);
          uint_t len = LDecode[lenIdx] + 2;
          if (const uint_t lenBits = LBits[lenIdx])
          {
            len += source.ReadBits(lenBits);
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
            dist += source.ReadBits(distBits);
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
      var.Dif[0] += Math::Absolute(idx);
      var.Dif[1] += Math::Absolute(idx - var.D[0]);
      var.Dif[2] += Math::Absolute(idx + var.D[0]);
      var.Dif[3] += Math::Absolute(idx - var.D[1]);
      var.Dif[4] += Math::Absolute(idx + var.D[1]);
      var.Dif[5] += Math::Absolute(idx - var.D[2]);
      var.Dif[6] += Math::Absolute(idx + var.D[2]);
      var.Dif[7] += Math::Absolute(idx - var.D[3]);
      var.Dif[8] += Math::Absolute(idx + var.D[3]);
      var.Dif[9] += Math::Absolute(idx - Audio.ChannelDelta);
      var.Dif[10] += Math::Absolute(idx + Audio.ChannelDelta);

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
        Dbg("Invalid backreference!");
      }
      History.Store(dist, count);
    }
  private:
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
    //target
    DecodeTarget Decoded;
  };

  class CompressedFile
  {
  public:
    typedef std::auto_ptr<const CompressedFile> Ptr;
    virtual ~CompressedFile() {}

    virtual Binary::Container::Ptr Decompress(const Container& container) const = 0;
  };

  class StoredFile : public CompressedFile
  {
  public:
    virtual Binary::Container::Ptr Decompress(const Container& container) const
    {
      const Formats::Packed::Rar::FileBlockHeader& header = container.GetHeader();
      const std::size_t offset = fromLE(header.Size);
      const std::size_t size = fromLE(header.AdditionalSize);
      const std::size_t outSize = fromLE(header.UnpackedSize);
      assert(0x30 == header.Method);
      if (size != outSize)
      {
        Dbg("Stored file sizes mismatch");
        return Binary::Container::Ptr();
      }
      else
      {
        Dbg("Restore");
        return container.GetData().GetSubcontainer(offset, size);
      }
    }
  };


  class PackedFile : public CompressedFile
  {
  public:
    virtual Binary::Container::Ptr Decompress(const Container& container) const
    {
      const Formats::Packed::Rar::FileBlockHeader& header = container.GetHeader();
      const std::size_t offset = fromLE(header.Size);
      const std::size_t size = fromLE(header.AdditionalSize);
      const std::size_t outSize = fromLE(header.UnpackedSize);
      assert(0x30 != header.Method);
      RarBitstream stream(container.GetField<uint8_t>(offset), size);
      Dbg("Depack %1% -> %2%", size, outSize);
      const bool isSolid = header.IsSolid();
      if (isSolid)
      {
        Dbg("solid mode on");
      }
      return Decoder.Decode(stream, outSize, isSolid);
    }
  private:
    mutable RarDecoder Decoder;
  };

  class DispatchedCompressedFile : public CompressedFile
  {
  public:
    DispatchedCompressedFile()
      : Packed(new PackedFile())
      , Stored(new StoredFile())
    {
    }

    virtual Binary::Container::Ptr Decompress(const Container& container) const
    {
      const Formats::Packed::Rar::FileBlockHeader& header = container.GetHeader();
      if (0x30 == header.Method)
      {
        return Stored->Decompress(container);
      }
      else
      {
        return Packed->Decompress(container);
      }
    }
  private:
    const std::auto_ptr<CompressedFile> Packed;
    const std::auto_ptr<CompressedFile> Stored;
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
        if (!Math::InRange<uint_t>(DepackerVersion, MIN_VERSION, MAX_VERSION))
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
        : Format(Binary::Format::Create(RarFile::HEADER_PATTERN))
        , Decoder(new RarFile::DispatchedCompressedFile())
      {
      }

      virtual String GetDescription() const
      {
        return Text::RAR_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const RarFile::Container container(rawData);
        if (!container.FastCheck())
        {
          return Container::Ptr();
        }
        return CreatePackedContainer(Decoder->Decompress(container), container.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Format;
      const RarFile::CompressedFile::Ptr Decoder;
    };

    Decoder::Ptr CreateRarDecoder()
    {
      return boost::make_shared<RarDecoder>();
    }
  }
}
