/**
 *
 * @file
 *
 * @brief  RAR archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
// library includes
#include <binary/container_base.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/archived.h>
#include <formats/packed/decoders.h>
#include <formats/packed/rar_supp.h>
// std includes
#include <cstring>
#include <deque>
#include <memory>
#include <numeric>

namespace Formats::Archived
{
  namespace Rar
  {
    const Debug::Stream Dbg("Formats::Archived::Rar");

    struct FileBlock
    {
      const Packed::Rar::FileBlockHeader* Header = nullptr;
      std::size_t Offset = 0;
      std::size_t Size = 0;

      FileBlock() = default;

      FileBlock(const Packed::Rar::FileBlockHeader* header, std::size_t offset, std::size_t size)
        : Header(header)
        , Offset(offset)
        , Size(size)
      {}

      std::size_t GetUnpackedSize() const
      {
        return Header->UnpackedSize;
      }

      bool HasParent() const
      {
        return Header->IsSolid();
      }

      bool IsChained() const
      {
        return !Header->IsStored();
      }
    };

    class BlocksIterator
    {
    public:
      explicit BlocksIterator(Binary::View data)
        : Stream(data)
      {}

      bool IsEof() const
      {
        const uint64_t curBlockSize = GetBlockSize();
        return !curBlockSize || curBlockSize > Stream.GetRestSize();
      }

      const Packed::Rar::ArchiveBlockHeader* GetArchiveHeader() const
      {
        assert(!IsEof());
        const auto* block = Stream.PeekField<Packed::Rar::ArchiveBlockHeader>();
        return block && !block->Block.IsExtended() && Packed::Rar::ArchiveBlockHeader::TYPE == block->Block.Type
                   ? block
                   : nullptr;
      }

      const Packed::Rar::FileBlockHeader* GetFileHeader() const
      {
        assert(!IsEof());
        const auto* block = Stream.PeekField<Packed::Rar::FileBlockHeader>();
        return block && block->IsValid() ? block : nullptr;
      }

      std::size_t GetOffset() const
      {
        return Stream.GetPosition();
      }

      uint64_t GetBlockSize() const
      {
        if (const auto* block = Stream.PeekField<Packed::Rar::BlockHeader>())
        {
          uint64_t res = block->Size;
          if (block->IsExtended())
          {
            if (const auto* extBlock = Stream.PeekField<Packed::Rar::ExtendedBlockHeader>())
            {
              res += extBlock->AdditionalSize;
              // Even if big files are not supported, we should properly skip them in stream
              if (Packed::Rar::FileBlockHeader::TYPE == extBlock->Block.Type
                  && Packed::Rar::FileBlockHeader::FLAG_BIG_FILE & extBlock->Block.Flags)
              {
                if (const auto* bigFile = Stream.PeekField<Packed::Rar::BigFileBlockHeader>())
                {
                  res += uint64_t(bigFile->PackedSizeHi) << (8 * sizeof(uint32_t));
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
        Stream.Skip(GetBlockSize());
        if (const auto* block = Stream.PeekField<Packed::Rar::BlockHeader>())
        {
          // finish block
          if (block->Type == 0x7b && 7 == block->Size)
          {
            Stream.Skip(sizeof(*block));
          }
        }
      }

    private:
      Binary::DataInputStream Stream;
    };

    class ChainDecoder
    {
    public:
      using Ptr = std::shared_ptr<const ChainDecoder>;

      explicit ChainDecoder(Binary::Container::Ptr data)
        : Data(std::move(data))
        , StatefulDecoder(Packed::CreateRarDecoder())
        , ChainIterator(new BlocksIterator(*Data))
      {}

      Binary::Container::Ptr DecodeBlock(const FileBlock& block) const
      {
        if (block.IsChained() && block.HasParent())
        {
          return AdvanceIterator(block.Offset, &ChainDecoder::ProcessBlock) ? DecodeSingleBlock(block)
                                                                            : Binary::Container::Ptr();
        }
        else
        {
          return AdvanceIterator(block.Offset, &ChainDecoder::SkipBlock) ? DecodeSingleBlock(block)
                                                                         : Binary::Container::Ptr();
        }
      }

    private:
      bool AdvanceIterator(std::size_t offset, void (ChainDecoder::*BlockOp)(const FileBlock&) const) const
      {
        if (ChainIterator->GetOffset() > offset)
        {
          Dbg(" Reset caching iterator to beginning");
          ChainIterator = std::make_unique<BlocksIterator>(*Data);
        }
        while (ChainIterator->GetOffset() <= offset && !ChainIterator->IsEof())
        {
          const FileBlock& curBlock =
              FileBlock(ChainIterator->GetFileHeader(), ChainIterator->GetOffset(), ChainIterator->GetBlockSize());
          ChainIterator->Next();
          if (curBlock.Header)
          {
            if (curBlock.Offset == offset)
            {
              return true;
            }
            else if (curBlock.IsChained())
            {
              (this->*BlockOp)(curBlock);
            }
          }
        }
        return false;
      }

      Binary::Container::Ptr DecodeSingleBlock(const FileBlock& block) const
      {
        Dbg(" Decoding block @{} (chained={}, hasParent={})", block.Offset, block.IsChained(), block.HasParent());
        const auto blockContent = Data->GetSubcontainer(block.Offset, block.Size);
        return StatefulDecoder->Decode(*blockContent);
      }

      void ProcessBlock(const FileBlock& block) const
      {
        Dbg(" Decoding parent block @{} (chained={}, hasParent={})", block.Offset, block.IsChained(),
            block.HasParent());
        const auto blockContent = Data->GetSubcontainer(block.Offset, block.Size);
        StatefulDecoder->Decode(*blockContent);
      }

      void SkipBlock(const FileBlock& block) const
      {
        Dbg(" Skip block @{} (chained={}, hasParent={})", block.Offset, block.IsChained(), block.HasParent());
      }

    private:
      const Binary::Container::Ptr Data;
      const Formats::Packed::Decoder::Ptr StatefulDecoder;
      mutable std::unique_ptr<BlocksIterator> ChainIterator;
    };

    class File : public Archived::File
    {
    public:
      File(ChainDecoder::Ptr decoder, FileBlock block, StringView name)
        : Decoder(std::move(decoder))
        , Block(block)
        , Name(name.to_string())
      {}

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return Block.GetUnpackedSize();
      }

      Binary::Container::Ptr GetData() const override
      {
        Dbg("Decompressing '{}' started at {}", Name, Block.Offset);
        return Decoder->DecodeBlock(Block);
      }

    private:
      const ChainDecoder::Ptr Decoder;
      const FileBlock Block;
      const String Name;
    };

    class FileIterator
    {
    public:
      FileIterator(ChainDecoder::Ptr decoder, const Binary::Container& data)
        : Decoder(std::move(decoder))
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
        const auto& file = *Blocks.GetFileHeader();
        return file.IsSupported();
      }

      String GetName() const
      {
        const auto& file = *Blocks.GetFileHeader();
        String name = file.GetName();
        std::replace(name.begin(), name.end(), '\\', '/');
        return name;
      }

      File::Ptr GetFile() const
      {
        const auto& file = *Blocks.GetFileHeader();
        if (file.IsSupported() && !Current)
        {
          const FileBlock block(&file, Blocks.GetOffset(), Blocks.GetBlockSize());
          Current = MakePtr<File>(Decoder, block, GetName());
        }
        return Current;
      }

      void Next()
      {
        assert(!IsEof());
        Current.reset();
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
      const ChainDecoder::Ptr Decoder;
      BlocksIterator Blocks;
      mutable File::Ptr Current;
    };

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      Container(Binary::Container::Ptr data, uint_t filesCount)
        : BaseContainer(std::move(data))
        , Decoder(MakePtr<ChainDecoder>(Delegate))
        , FilesCount(filesCount)
      {
        Dbg("Found {} files. Size is {}", filesCount, Delegate->Size());
      }

      // Archive::Container
      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (FileIterator iter(Decoder, *Delegate); !iter.IsEof(); iter.Next())
        {
          if (!iter.IsValid())
          {
            continue;
          }
          const File::Ptr fileObject = iter.GetFile();
          walker.OnFile(*fileObject);
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        for (FileIterator iter(Decoder, *Delegate); !iter.IsEof(); iter.Next())
        {
          if (iter.IsValid() && iter.GetName() == name)
          {
            return iter.GetFile();
          }
        }
        return {};
      }

      uint_t CountFiles() const override
      {
        return FilesCount;
      }

    private:
      const ChainDecoder::Ptr Decoder;
      const uint_t FilesCount;
    };

    const Char DESCRIPTION[] = "RAR";
    const auto FORMAT =
        // file marker
        "5261"  // uint16_t CRC;   "Ra"
        "72"    // uint8_t Type;   "r"
        "211a"  // uint16_t Flags; "!ESC^"
        "0700"  // uint16_t Size;  "BELL^,0"
        // archive header
        "??"    // uint16_t CRC;
        "73"    // uint8_t Type;
        "??"    // uint16_t Flags;
        "0d00"  // uint16_t Size
        ""_sv;
  }  // namespace Rar

  class RarDecoder : public Decoder
  {
  public:
    RarDecoder()
      : Format(Binary::CreateFormat(Rar::FORMAT))
    {}

    String GetDescription() const override
    {
      return Rar::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      if (!Format->Match(data))
      {
        return {};
      }

      uint_t filesCount = 0;
      Rar::BlocksIterator iter(data);
      for (; !iter.IsEof(); iter.Next())
      {
        if (const Formats::Packed::Rar::FileBlockHeader* file = iter.GetFileHeader())
        {
          filesCount += file->IsSupported();
        }
      }
      if (const std::size_t totalSize = iter.GetOffset())
      {
        const Binary::Container::Ptr archive = data.GetSubcontainer(0, totalSize);
        return MakePtr<Rar::Container>(archive, filesCount);
      }
      else
      {
        return {};
      }
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateRarDecoder()
  {
    return MakePtr<RarDecoder>();
  }
}  // namespace Formats::Archived
