/**
 *
 * @file
 *
 * @brief  ZX-State snapshots support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/zxstate_supp.h"
// common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
// library includes
#include <binary/compression/zlib_container.h>
#include <binary/container_base.h>
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/archived.h>
#include <strings/format.h>
// std includes
#include <cstring>
#include <list>
#include <map>
#include <numeric>
#include <sstream>
// boost includes
#include <boost/range/size.hpp>

namespace Formats::Archived
{
  namespace ZXState
  {
    const Debug::Stream Dbg("Formats::Archived::ZXState");

    const Char DESCRIPTION[] = "SZX (ZX-State)";
    const auto FORMAT =
        "'Z'X'S'T"   // signature
        "01"         // major
        "00-04"      // minor
        "00-10"      // machineId
        "%0000000x"  // flags
        ""_sv;

    struct DataBlockDescription
    {
      const void* Content;
      std::size_t Size;
      bool IsCompressed;
      std::size_t UncompressedSize;

      DataBlockDescription()
        : Content()
        , Size()
        , IsCompressed()
        , UncompressedSize()
      {}
    };

    class ChunksVisitor
    {
    public:
      virtual ~ChunksVisitor() = default;

      virtual bool Visit(const Chunk& ch) = 0;
      virtual bool Visit(const Chunk& ch, const DataBlockDescription& blk) = 0;
      virtual bool Visit(const Chunk& ch, uint_t idx, const DataBlockDescription& blk) = 0;
      virtual bool Visit(const Chunk& ch, const String& suffix, const DataBlockDescription& blk) = 0;
    };

    const Char RAM_SUFFIX[] = {'R', 'A', 'M', 0};
    const Char ROM_SUFFIX[] = {'R', 'O', 'M', 0};

    class ChunksSet
    {
    public:
      explicit ChunksSet(const Binary::Container& data)
        : Data(data)
      {}

      std::size_t Parse(ChunksVisitor& visitor) const
      {
        ChunksIterator iterator(Data);
        for (std::size_t size = iterator.GetPosition();; size = iterator.GetPosition())
        {
          if (const Chunk* cur = iterator.GetNext())
          {
            if (ParseChunk(*cur, visitor))
            {
              continue;
            }
          }
          return size;
        }
        return 0;
      }

    private:
      static bool ParseChunk(const Chunk& cur, ChunksVisitor& visitor)
      {
        if (const ChunkATASP* atasp = cur.IsA<ChunkATASP>())
        {
          return visitor.Visit(*atasp);
        }
        if (const ChunkATASPRAM* ataspram = cur.IsA<ChunkATASPRAM>())
        {
          return ParseMultiBlockChunk(*ataspram, visitor);
        }
        else if (const ChunkAYBLOCK* ayblock = cur.IsA<ChunkAYBLOCK>())
        {
          return visitor.Visit(*ayblock);
        }
        else if (const ChunkBETA128* beta128 = cur.IsA<ChunkBETA128>())
        {
          return ParseSingleBlockChunk(*beta128, visitor);
        }
        else if (const ChunkBETADISK* betadisk = cur.IsA<ChunkBETADISK>())
        {
          return ParseMultiBlockChunk(*betadisk, visitor);
        }
        else if (const ChunkCF* cf = cur.IsA<ChunkCF>())
        {
          return visitor.Visit(*cf);
        }
        else if (const ChunkCFRAM* cfram = cur.IsA<ChunkCFRAM>())
        {
          return ParseMultiBlockChunk(*cfram, visitor);
        }
        else if (const ChunkCOVOX* covox = cur.IsA<ChunkCOVOX>())
        {
          return visitor.Visit(*covox);
        }
        else if (const ChunkCREATOR* creator = cur.IsA<ChunkCREATOR>())
        {
          return visitor.Visit(*creator);
        }
        else if (const ChunkDIVIDE* divide = cur.IsA<ChunkDIVIDE>())
        {
          return ParseSingleBlockChunk(*divide, visitor);
        }
        else if (const ChunkDIVIDERAMPAGE* dividerampage = cur.IsA<ChunkDIVIDERAMPAGE>())
        {
          return ParseMultiBlockChunk(*dividerampage, visitor);
        }
        else if (const ChunkDOCK* dock = cur.IsA<ChunkDOCK>())
        {
          return ParseMultiBlockChunk(*dock, visitor);
        }
        else if (const ChunkDSKFILE* dskfile = cur.IsA<ChunkDSKFILE>())
        {
          return ParseMultiBlockChunk(*dskfile, visitor);
        }
        else if (const ChunkGS* gs = cur.IsA<ChunkGS>())
        {
          return ParseSingleBlockChunk(*gs, visitor);
        }
        else if (const ChunkGSRAMPAGE* gsrampage = cur.IsA<ChunkGSRAMPAGE>())
        {
          return ParseMultiBlockChunk(*gsrampage, visitor);
        }
        else if (const ChunkIF1* if1 = cur.IsA<ChunkIF1>())
        {
          return ParseSingleBlockChunk(*if1, visitor);
        }
        else if (const ChunkIF2ROM* if2rom = cur.IsA<ChunkIF2ROM>())
        {
          return ParseSingleBlockChunk(*if2rom, visitor);
        }
        else if (const ChunkJOYSTICK* joystick = cur.IsA<ChunkJOYSTICK>())
        {
          return visitor.Visit(*joystick);
        }
        else if (const ChunkKEYBOARD* keyboard = cur.IsA<ChunkKEYBOARD>())
        {
          return visitor.Visit(*keyboard);
        }
        else if (const ChunkMCART* mcart = cur.IsA<ChunkMCART>())
        {
          return ParseMultiBlockChunk(*mcart, visitor);
        }
        else if (const ChunkMOUSE* mouse = cur.IsA<ChunkMOUSE>())
        {
          return visitor.Visit(*mouse);
        }
        else if (const ChunkMULTIFACE* multiface = cur.IsA<ChunkMULTIFACE>())
        {
          return ParseSingleBlockChunk(*multiface, visitor);
        }
        else if (const ChunkOPUS* opus = cur.IsA<ChunkOPUS>())
        {
          return ParseMergedBlocksChunk(*opus, visitor);
        }
        else if (const ChunkOPUSDISK* opusdisk = cur.IsA<ChunkOPUSDISK>())
        {
          return ParseMultiBlockChunk(*opusdisk, visitor);
        }
        else if (const ChunkPLTT* pltt = cur.IsA<ChunkPLTT>())
        {
          return visitor.Visit(*pltt);
        }
        else if (const ChunkPLUS3* plus3 = cur.IsA<ChunkPLUS3>())
        {
          return visitor.Visit(*plus3);
        }
        else if (const ChunkPLUSD* plusd = cur.IsA<ChunkPLUSD>())
        {
          return ParseMergedBlocksChunk(*plusd, visitor);
        }
        else if (const ChunkPLUSDDISK* plusddisk = cur.IsA<ChunkPLUSDDISK>())
        {
          return ParseMultiBlockChunk(*plusddisk, visitor);
        }
        else if (const ChunkRAMPAGE* rampage = cur.IsA<ChunkRAMPAGE>())
        {
          return ParseMultiBlockChunk(*rampage, visitor);
        }
        else if (const ChunkROM* rom = cur.IsA<ChunkROM>())
        {
          return ParseSingleBlockChunk(*rom, visitor);
        }
        else if (const ChunkSCLD* scld = cur.IsA<ChunkSCLD>())
        {
          return visitor.Visit(*scld);
        }
        else if (const ChunkSIDE* side = cur.IsA<ChunkSIDE>())
        {
          return visitor.Visit(*side);
        }
        else if (const ChunkSPECDRUM* specdrum = cur.IsA<ChunkSPECDRUM>())
        {
          return visitor.Visit(*specdrum);
        }
        else if (const ChunkSPECREGS* specregs = cur.IsA<ChunkSPECREGS>())
        {
          return visitor.Visit(*specregs);
        }
        else if (const ChunkSPECTRANET* spectranet = cur.IsA<ChunkSPECTRANET>())
        {
          return ParseMergedBlocksChunk(*spectranet, visitor);
        }
        else if (const ChunkTAPE* tape = cur.IsA<ChunkTAPE>())
        {
          return ParseMultiBlockChunk(*tape, visitor);
        }
        else if (const ChunkUSPEECH* uspeech = cur.IsA<ChunkUSPEECH>())
        {
          return visitor.Visit(*uspeech);
        }
        else if (const ChunkZXPRINTER* zxprinter = cur.IsA<ChunkZXPRINTER>())
        {
          return visitor.Visit(*zxprinter);
        }
        else if (const ChunkZ80REGS* z80regs = cur.IsA<ChunkZ80REGS>())
        {
          return visitor.Visit(*z80regs);
        }
        else  // unknown chunk
        {
          return false;
        }
      }

      static bool ParseSingleBlockChunk(const ChunkIF1& ch, ChunksVisitor& visitor)
      {
        typedef ChunkTraits<ChunkIF1> Traits;
        if (const std::size_t targetSize = Traits::GetTargetSize(ch))
        {
          if (targetSize != 0x4000 && targetSize != 0x2000)
          {
            // invalid size
            return false;
          }
          DataBlockDescription blk;
          blk.Content = Traits::GetData(ch);
          blk.Size = Traits::GetDataSize(ch);
          blk.IsCompressed = Traits::IsDataCompressed(ch);
          blk.UncompressedSize = targetSize;
          return visitor.Visit(ch, blk);
        }
        else
        {
          return true;
        }
      }

      static bool ParseSingleBlockChunk(const ChunkIF2ROM& ch, ChunksVisitor& visitor)
      {
        typedef ChunkTraits<ChunkIF2ROM> Traits;
        DataBlockDescription blk;
        blk.Content = Traits::GetData(ch);
        blk.Size = Traits::GetDataSize(ch);
        blk.IsCompressed = true;
        blk.UncompressedSize = Traits::GetTargetSize(ch);
        return visitor.Visit(ch, blk);
      }

      template<class ChunkType>
      static bool ParseSingleBlockChunk(const ChunkType& ch, ChunksVisitor& visitor)
      {
        typedef ChunkTraits<ChunkType> Traits;
        if (const std::size_t targetSize = Traits::GetTargetSize(ch))
        {
          DataBlockDescription blk;
          blk.Content = Traits::GetData(ch);
          blk.Size = Traits::GetDataSize(ch);
          blk.IsCompressed = Traits::IsDataCompressed(ch);
          blk.UncompressedSize = targetSize;
          return visitor.Visit(ch, blk);
        }
        else
        {
          return true;
        }
      }

      static bool ParseMergedBlocksChunk(const ChunkSPECTRANET& ch, ChunksVisitor& visitor)
      {
        DataBlockDescription romBlk;
        romBlk.Content = ch.Data;
        romBlk.Size = ch.FlashSize;
        romBlk.IsCompressed = 0 != (ch.Flags & ch.COMPRESSED);
        romBlk.UncompressedSize = ch.DUMPSIZE;
        if (!visitor.Visit(ch, ROM_SUFFIX, romBlk))
        {
          return false;
        }
        const auto* const romDescr = safe_ptr_cast<const le_uint32_t*>(ch.Data + romBlk.Size);
        DataBlockDescription ramBlk;
        ramBlk.Content = romDescr + 1;
        ramBlk.Size = *romDescr;
        ramBlk.IsCompressed = 0 != (ch.Flags & ch.COMPRESSED_RAM);
        ramBlk.UncompressedSize = ch.DUMPSIZE;
        return visitor.Visit(ch, RAM_SUFFIX, ramBlk);
      }

      template<class ChunkType>
      static bool ParseMergedBlocksChunk(const ChunkType& ch, ChunksVisitor& visitor)
      {
        typedef ChunkTraits<ChunkType> Traits;
        DataBlockDescription ramBlk;
        ramBlk.Content = Traits::GetData(ch);
        ramBlk.Size = ch.RamDataSize;
        ramBlk.IsCompressed = Traits::IsDataCompressed(ch);
        ramBlk.UncompressedSize = ch.RAMSIZE;
        if (!visitor.Visit(ch, RAM_SUFFIX, ramBlk))
        {
          return false;
        }
        if (0 != (ch.Flags & ch.CUSTOMROM))
        {
          DataBlockDescription romBlk;
          romBlk.Content = Traits::GetData(ch) + ramBlk.Size;
          romBlk.Size = ch.RomDataSize;
          romBlk.IsCompressed = Traits::IsDataCompressed(ch);
          romBlk.UncompressedSize = ch.ROMSIZE;
          if (!visitor.Visit(ch, ROM_SUFFIX, romBlk))
          {
            return false;
          }
        }
        return true;
      }

      template<class ChunkType>
      static bool ParseMultiBlockChunk(const ChunkType& ch, ChunksVisitor& visitor)
      {
        typedef ChunkTraits<ChunkType> Traits;
        if (const std::size_t targetSize = Traits::GetTargetSize(ch))
        {
          DataBlockDescription blk;
          blk.Content = Traits::GetData(ch);
          blk.Size = Traits::GetDataSize(ch);
          blk.IsCompressed = Traits::IsDataCompressed(ch);
          blk.UncompressedSize = targetSize;
          return visitor.Visit(ch, Traits::GetIndex(ch), blk);
        }
        else
        {
          return true;
        }
      }

    private:
      class ChunksIterator
      {
      public:
        explicit ChunksIterator(const Binary::Container& container)
          : Stream(container)
        {
          const auto& hdr = Stream.Read<Header>();
          Require(hdr.Id == Header::SIGNATURE);
          Dbg("ZXState container ver {}.{}", uint_t(hdr.Major), uint_t(hdr.Minor));
        }

        const Chunk* GetNext()
        {
          const std::size_t rest = Stream.GetRestSize();
          if (rest < sizeof(Chunk))
          {
            return nullptr;
          }
          const auto& chunk = Stream.Read<Chunk>();
          const std::size_t chunkSize = chunk.Size;
          if (rest < sizeof(Chunk) + chunkSize)
          {
            return nullptr;
          }
          Stream.Skip(chunkSize);
          return &chunk;
        }

        std::size_t GetPosition() const
        {
          return Stream.GetPosition();
        }

      private:
        Binary::InputStream Stream;
      };

    private:
      const Binary::Container& Data;
    };

    const std::size_t MAX_DECOMPRESS_SIZE = 2 * 1048576;  // 2M

    Binary::Container::Ptr DecompressData(const DataBlockDescription& blk)
    {
      Require(blk.Content && blk.IsCompressed);
      const std::size_t targetSize = blk.UncompressedSize == UNKNOWN ? 0 : blk.UncompressedSize;
      try
      {
        return Binary::Compression::Zlib::Decompress(Binary::View(blk.Content, blk.Size), targetSize);
      }
      catch (const Error& e)
      {
        Dbg("Failed to decompress: {}", e.ToString());
      }
      catch (const std::exception&)
      {
        Dbg("Failed to decompress");
      }
      return Binary::Container::Ptr();
    }

    Binary::Container::Ptr ExtractData(const DataBlockDescription& blk)
    {
      if (const void* src = blk.Content)
      {
        if (blk.IsCompressed)
        {
          return DecompressData(blk);
        }
        else
        {
          Require(blk.Size == blk.UncompressedSize);
          return Binary::CreateContainer(Binary::View(src, blk.Size));
        }
      }
      else
      {
        return Binary::Container::Ptr();
      }
    }

    class SingleBlockFile : public Archived::File
    {
    public:
      SingleBlockFile(Binary::Container::Ptr archive, String name, DataBlockDescription block)
        : Data(std::move(archive))
        , Name(std::move(name))
        , Block(std::move(block))
      {
        Dbg("Created file '{}', size={}, packed size={}, compression={}", Name, Block.UncompressedSize, Block.Size,
            Block.IsCompressed);
      }

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return Block.UncompressedSize;
      }

      Binary::Container::Ptr GetData() const override
      {
        Dbg("Decompressing '{}' ({} -> {})", Name, Block.Size, Block.UncompressedSize);
        return ExtractData(Block);
      }

    private:
      const Binary::Container::Ptr Data;
      const String Name;
      const DataBlockDescription Block;
    };

    typedef std::vector<DataBlockDescription> DataBlocks;

    std::size_t SumBlocksSize(std::size_t sum, const DataBlockDescription& descr)
    {
      return sum + descr.UncompressedSize;
    }

    class MultiBlockFile : public Archived::File
    {
    public:
      MultiBlockFile(Binary::Container::Ptr archive, String name, DataBlocks blocks)
        : Data(std::move(archive))
        , Name(std::move(name))
        , Blocks(std::move(blocks))
      {
        Dbg("Created file '{}', contains from {} parts", Name, Blocks.size());
      }

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return std::accumulate(Blocks.begin(), Blocks.end(), std::size_t(0), &SumBlocksSize);
      }

      Binary::Container::Ptr GetData() const override
      {
        try
        {
          const std::size_t unpacked = GetSize();
          Require(unpacked != 0);
          Dbg("Decompressing '{}' ({} blocks, {} butes result)", Name, Blocks.size(), unpacked);
          std::unique_ptr<Binary::Dump> result(new Binary::Dump(unpacked));
          auto* target = result->data();
          for (const auto& block : Blocks)
          {
            const Binary::Container::Ptr data = ExtractData(block);
            Require(data && data->Size() == block.UncompressedSize);
            std::memcpy(target, data->Start(), block.UncompressedSize);
            target += block.UncompressedSize;
          }
          return Binary::CreateContainer(std::move(result));
        }
        catch (const std::exception&)
        {
          Dbg("Failed to decompress");
          return Binary::Container::Ptr();
        }
      }

    private:
      const Binary::Container::Ptr Data;
      const String Name;
      const DataBlocks Blocks;
    };

    String GenerateChunkName(const Chunk& ch)
    {
      std::array<char, sizeof(ch.Id)> syms;
      std::memcpy(syms.data(), &ch.Id, sizeof(ch.Id));
      return String(syms.data(), syms.size());
    }

    template<class T>
    String GenerateChunkName(const Chunk& ch, const T& suffix)
    {
      const String base = GenerateChunkName(ch);
      if (ch.Id == ChunkATASPRAM::SIGNATURE || ch.Id == ChunkCFRAM::SIGNATURE || ch.Id == ChunkDIVIDERAMPAGE::SIGNATURE
          || ch.Id == ChunkDOCK::SIGNATURE || ch.Id == ChunkGSRAMPAGE::SIGNATURE || ch.Id == ChunkRAMPAGE::SIGNATURE)
      {
        return base;
      }
      else
      {
        std::basic_ostringstream<Char> str;
        str << base << Char('.') << suffix;
        return str.str();
      }
    }

    typedef std::map<String, DataBlocks> NamedBlocksMap;

    class FilledBlocks
      : public NamedBlocksMap
      , public ChunksVisitor
    {
    public:
      bool Visit(const Chunk& ch) override
      {
        Dbg("Skipping useless '{}'", GenerateChunkName(ch));
        return true;
      }

      bool Visit(const Chunk& ch, const DataBlockDescription& blk) override
      {
        const String& name = GenerateChunkName(ch);
        Dbg("Single block '{}'", name);
        (*this)[name].push_back(blk);
        return true;
      }

      bool Visit(const Chunk& ch, uint_t idx, const DataBlockDescription& blk) override
      {
        const String& name = GenerateChunkName(ch, idx);
        Dbg("Single indexed block '{}'", name);
        DataBlocksAdapter blocks((*this)[name], ch.Id);
        blocks.Add(idx, blk);
        return true;
      }

      bool Visit(const Chunk& ch, const String& suffix, const DataBlockDescription& blk) override
      {
        const String& name = GenerateChunkName(ch, suffix);
        Dbg("Single suffixed block '{}'", name);
        (*this)[name].push_back(blk);
        return true;
      }

    private:
      class DataBlocksAdapter
      {
      public:
        DataBlocksAdapter(DataBlocks& delegate, const Identifier& type)
          : Delegate(delegate)
          , Rampages(type == ChunkRAMPAGE::SIGNATURE)
        {}

        void Add(uint_t idx, const DataBlockDescription& blk)
        {
          const std::size_t orderNum = GetOrderNum(idx);
          if (Delegate.size() < orderNum + 1)
          {
            Delegate.resize(orderNum + 1);
          }
          Delegate[orderNum] = blk;
        }

      private:
        std::size_t GetOrderNum(uint_t idx) const
        {
          static const std::size_t RAMPAGES[] = {2, 3, 1, 4, 5, 0};
          return Rampages && idx < boost::size(RAMPAGES) ? RAMPAGES[idx] : idx;
        }

      private:
        DataBlocks& Delegate;
        const bool Rampages;
      };
    };

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      Container(Binary::Container::Ptr archive, NamedBlocksMap blocks)
        : BaseContainer(std::move(archive))
        , Blocks(std::move(blocks))
      {}

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (const auto& block : Blocks)
        {
          const File::Ptr file = CreateFileOnBlocks(block.first, block.second);
          walker.OnFile(*file);
        }
      }

      File::Ptr FindFile(const String& name) const override
      {
        const NamedBlocksMap::const_iterator it = Blocks.find(name);
        return it != Blocks.end() ? CreateFileOnBlocks(it->first, it->second) : File::Ptr();
      }

      uint_t CountFiles() const override
      {
        return static_cast<uint_t>(Blocks.size());
      }

    private:
      File::Ptr CreateFileOnBlocks(const String& name, const DataBlocks& blocks) const
      {
        if (blocks.size() == 1)
        {
          return MakePtr<SingleBlockFile>(Delegate, name, blocks.front());
        }
        else
        {
          return MakePtr<MultiBlockFile>(Delegate, name, blocks);
        }
      }

    private:
      const NamedBlocksMap Blocks;
    };
  }  // namespace ZXState

  class ZXStateDecoder : public Decoder
  {
  public:
    ZXStateDecoder()
      : Format(Binary::CreateFormat(ZXState::FORMAT))
    {}

    String GetDescription() const override
    {
      return ZXState::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      using namespace ZXState;
      if (!Format->Match(data))
      {
        return ZXState::Container::Ptr();
      }
      const ChunksSet chunks(data);
      FilledBlocks blocks;
      if (const std::size_t size = chunks.Parse(blocks))
      {
        if (!blocks.empty())
        {
          const Binary::Container::Ptr archive = data.GetSubcontainer(0, size);
          return MakePtr<ZXState::Container>(archive, blocks);
        }
        Dbg("No files found");
      }
      return ZXState::Container::Ptr();
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateZXStateDecoder()
  {
    return MakePtr<ZXStateDecoder>();
  }
}  // namespace Formats::Archived
