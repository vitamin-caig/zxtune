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
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/archived.h>
#include <strings/format.h>
#include <strings/map.h>
// std includes
#include <cstring>
#include <list>
#include <numeric>
#include <sstream>

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
      const void* Content = nullptr;
      std::size_t Size = 0;
      bool IsCompressed = false;
      std::size_t UncompressedSize = 0;

      DataBlockDescription() = default;
    };

    class ChunksVisitor
    {
    public:
      virtual ~ChunksVisitor() = default;

      virtual bool Visit(const Chunk& ch) = 0;
      virtual bool Visit(const Chunk& ch, const DataBlockDescription& blk) = 0;
      virtual bool Visit(const Chunk& ch, uint_t idx, const DataBlockDescription& blk) = 0;
      virtual bool Visit(const Chunk& ch, StringView suffix, const DataBlockDescription& blk) = 0;
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
        if (const auto* atasp = cur.IsA<ChunkATASP>())
        {
          return visitor.Visit(*atasp);
        }
        if (const auto* ataspram = cur.IsA<ChunkATASPRAM>())
        {
          return ParseMultiBlockChunk(*ataspram, visitor);
        }
        else if (const auto* ayblock = cur.IsA<ChunkAYBLOCK>())
        {
          return visitor.Visit(*ayblock);
        }
        else if (const auto* beta128 = cur.IsA<ChunkBETA128>())
        {
          return ParseSingleBlockChunk(*beta128, visitor);
        }
        else if (const auto* betadisk = cur.IsA<ChunkBETADISK>())
        {
          return ParseMultiBlockChunk(*betadisk, visitor);
        }
        else if (const auto* cf = cur.IsA<ChunkCF>())
        {
          return visitor.Visit(*cf);
        }
        else if (const auto* cfram = cur.IsA<ChunkCFRAM>())
        {
          return ParseMultiBlockChunk(*cfram, visitor);
        }
        else if (const auto* covox = cur.IsA<ChunkCOVOX>())
        {
          return visitor.Visit(*covox);
        }
        else if (const auto* creator = cur.IsA<ChunkCREATOR>())
        {
          return visitor.Visit(*creator);
        }
        else if (const auto* divide = cur.IsA<ChunkDIVIDE>())
        {
          return ParseSingleBlockChunk(*divide, visitor);
        }
        else if (const auto* dividerampage = cur.IsA<ChunkDIVIDERAMPAGE>())
        {
          return ParseMultiBlockChunk(*dividerampage, visitor);
        }
        else if (const auto* dock = cur.IsA<ChunkDOCK>())
        {
          return ParseMultiBlockChunk(*dock, visitor);
        }
        else if (const auto* dskfile = cur.IsA<ChunkDSKFILE>())
        {
          return ParseMultiBlockChunk(*dskfile, visitor);
        }
        else if (const auto* gs = cur.IsA<ChunkGS>())
        {
          return ParseSingleBlockChunk(*gs, visitor);
        }
        else if (const auto* gsrampage = cur.IsA<ChunkGSRAMPAGE>())
        {
          return ParseMultiBlockChunk(*gsrampage, visitor);
        }
        else if (const auto* if1 = cur.IsA<ChunkIF1>())
        {
          return ParseSingleBlockChunk(*if1, visitor);
        }
        else if (const auto* if2rom = cur.IsA<ChunkIF2ROM>())
        {
          return ParseSingleBlockChunk(*if2rom, visitor);
        }
        else if (const auto* joystick = cur.IsA<ChunkJOYSTICK>())
        {
          return visitor.Visit(*joystick);
        }
        else if (const auto* keyboard = cur.IsA<ChunkKEYBOARD>())
        {
          return visitor.Visit(*keyboard);
        }
        else if (const auto* mcart = cur.IsA<ChunkMCART>())
        {
          return ParseMultiBlockChunk(*mcart, visitor);
        }
        else if (const auto* mouse = cur.IsA<ChunkMOUSE>())
        {
          return visitor.Visit(*mouse);
        }
        else if (const auto* multiface = cur.IsA<ChunkMULTIFACE>())
        {
          return ParseSingleBlockChunk(*multiface, visitor);
        }
        else if (const auto* opus = cur.IsA<ChunkOPUS>())
        {
          return ParseMergedBlocksChunk(*opus, visitor);
        }
        else if (const auto* opusdisk = cur.IsA<ChunkOPUSDISK>())
        {
          return ParseMultiBlockChunk(*opusdisk, visitor);
        }
        else if (const auto* pltt = cur.IsA<ChunkPLTT>())
        {
          return visitor.Visit(*pltt);
        }
        else if (const auto* plus3 = cur.IsA<ChunkPLUS3>())
        {
          return visitor.Visit(*plus3);
        }
        else if (const auto* plusd = cur.IsA<ChunkPLUSD>())
        {
          return ParseMergedBlocksChunk(*plusd, visitor);
        }
        else if (const auto* plusddisk = cur.IsA<ChunkPLUSDDISK>())
        {
          return ParseMultiBlockChunk(*plusddisk, visitor);
        }
        else if (const auto* rampage = cur.IsA<ChunkRAMPAGE>())
        {
          return ParseMultiBlockChunk(*rampage, visitor);
        }
        else if (const auto* rom = cur.IsA<ChunkROM>())
        {
          return ParseSingleBlockChunk(*rom, visitor);
        }
        else if (const auto* scld = cur.IsA<ChunkSCLD>())
        {
          return visitor.Visit(*scld);
        }
        else if (const auto* side = cur.IsA<ChunkSIDE>())
        {
          return visitor.Visit(*side);
        }
        else if (const auto* specdrum = cur.IsA<ChunkSPECDRUM>())
        {
          return visitor.Visit(*specdrum);
        }
        else if (const auto* specregs = cur.IsA<ChunkSPECREGS>())
        {
          return visitor.Visit(*specregs);
        }
        else if (const auto* spectranet = cur.IsA<ChunkSPECTRANET>())
        {
          return ParseMergedBlocksChunk(*spectranet, visitor);
        }
        else if (const auto* tape = cur.IsA<ChunkTAPE>())
        {
          return ParseMultiBlockChunk(*tape, visitor);
        }
        else if (const auto* uspeech = cur.IsA<ChunkUSPEECH>())
        {
          return visitor.Visit(*uspeech);
        }
        else if (const auto* zxprinter = cur.IsA<ChunkZXPRINTER>())
        {
          return visitor.Visit(*zxprinter);
        }
        else if (const auto* z80regs = cur.IsA<ChunkZ80REGS>())
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
        using Traits = ChunkTraits<ChunkIF1>;
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
        using Traits = ChunkTraits<ChunkIF2ROM>;
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
        using Traits = ChunkTraits<ChunkType>;
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
        using Traits = ChunkTraits<ChunkType>;
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
        using Traits = ChunkTraits<ChunkType>;
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
      return {};
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
        return {};
      }
    }

    class SingleBlockFile : public Archived::File
    {
    public:
      SingleBlockFile(Binary::Container::Ptr archive, StringView name, DataBlockDescription block)
        : Data(std::move(archive))
        , Name(name.to_string())
        , Block(block)
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

    using DataBlocks = std::vector<DataBlockDescription>;

    std::size_t SumBlocksSize(std::size_t sum, const DataBlockDescription& descr)
    {
      return sum + descr.UncompressedSize;
    }

    class MultiBlockFile : public Archived::File
    {
    public:
      MultiBlockFile(Binary::Container::Ptr archive, StringView name, DataBlocks blocks)
        : Data(std::move(archive))
        , Name(name.to_string())
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
          Binary::DataBuilder result(unpacked);
          for (const auto& block : Blocks)
          {
            const auto data = ExtractData(block);
            Require(data && data->Size() == block.UncompressedSize);
            result.Add(*data);
          }
          return result.CaptureResult();
        }
        catch (const std::exception&)
        {
          Dbg("Failed to decompress");
          return {};
        }
      }

    private:
      const Binary::Container::Ptr Data;
      const String Name;
      const DataBlocks Blocks;
    };

    // TODO: StringView
    String GenerateChunkName(const Chunk& ch)
    {
      std::array<char, sizeof(ch.Id)> syms;
      std::memcpy(syms.data(), &ch.Id, sizeof(ch.Id));
      return {syms.data(), syms.size()};
    }

    template<class T>
    String GenerateChunkName(const Chunk& ch, const T& suffix)
    {
      auto base = GenerateChunkName(ch);
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

    using NamedBlocksMap = Strings::ValueMap<DataBlocks>;

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
        const auto& name = GenerateChunkName(ch);
        Dbg("Single block '{}'", name);
        (*this)[name].push_back(blk);
        return true;
      }

      bool Visit(const Chunk& ch, uint_t idx, const DataBlockDescription& blk) override
      {
        const auto& name = GenerateChunkName(ch, idx);
        Dbg("Single indexed block '{}'", name);
        DataBlocksAdapter blocks((*this)[name], ch.Id);
        blocks.Add(idx, blk);
        return true;
      }

      bool Visit(const Chunk& ch, StringView suffix, const DataBlockDescription& blk) override
      {
        const auto& name = GenerateChunkName(ch, suffix);
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
          return Rampages && idx < std::size(RAMPAGES) ? RAMPAGES[idx] : idx;
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
          const auto file = CreateFileOnBlocks(block.first, block.second);
          walker.OnFile(*file);
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        if (const auto* ptr = Blocks.FindPtr(name))
        {
          return CreateFileOnBlocks(name, *ptr);
        }
        else
        {
          return {};
        }
      }

      uint_t CountFiles() const override
      {
        return static_cast<uint_t>(Blocks.size());
      }

    private:
      File::Ptr CreateFileOnBlocks(StringView name, const DataBlocks& blocks) const
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
        return {};
      }
      const ChunksSet chunks(data);
      FilledBlocks blocks;
      if (const std::size_t size = chunks.Parse(blocks))
      {
        if (!blocks.empty())
        {
          auto archive = data.GetSubcontainer(0, size);
          return MakePtr<ZXState::Container>(std::move(archive), blocks);
        }
        Dbg("No files found");
      }
      return {};
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateZXStateDecoder()
  {
    return MakePtr<ZXStateDecoder>();
  }
}  // namespace Formats::Archived
