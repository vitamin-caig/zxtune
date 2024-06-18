/**
 *
 * @file
 *
 * @brief  PSF2 VFS parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/emulation/playstation2soundformat.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/compression/zlib_container.h>
#include <binary/container_factories.h>
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
// std includes
#include <map>

namespace Formats::Chiptune
{
  namespace Playstation2SoundFormat
  {
    const Debug::Stream Dbg("Formats::Chiptune::PSF2");

    const Char DESCRIPTION[] = "Playstation2 Sound Format";

    struct DirectoryEntry
    {
      static const std::size_t RAW_SIZE = 48;

      String Name;
      std::size_t Offset;
      std::size_t Size;
      std::size_t BlockSize;

      explicit DirectoryEntry(Binary::InputStream& stream)
      {
        const auto* const nameBegin = stream.PeekRawData(36);
        const auto* const nameEnd = std::find(nameBegin, nameBegin + 36, 0);
        Name.assign(nameBegin, nameEnd);
        stream.Skip(36);
        Offset = stream.Read<le_uint32_t>();
        Size = stream.Read<le_uint32_t>();
        BlockSize = stream.Read<le_uint32_t>();
      }

      bool IsDir() const
      {
        return Offset != 0 && Size == 0 && BlockSize == 0;
      }
    };

    using FileBlocks = std::map<std::size_t, Binary::Container::Ptr>;

    class ScatteredContainer : public Binary::Container
    {
    public:
      ScatteredContainer(FileBlocks blocks, std::size_t totalSize)
        : TotalSize(totalSize)
        , Blocks(std::move(blocks))
      {
        Require(Blocks.size() > 1);
      }

      const void* Start() const override
      {
        return Merge().Start();
      }

      std::size_t Size() const override
      {
        return TotalSize;
      }

      Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
      {
        if (!Merged)
        {
          const auto it = Blocks.lower_bound(offset);
          if (it == Blocks.end() || it->first + it->second->Size() <= offset)
          {
            return {};
          }
          const auto& part = it->second;
          const auto partOffset = offset - it->first;
          const auto partSize = part->Size();
          if (!partOffset && partSize == size)
          {
            return part;
          }
          else if (partOffset + size <= partSize)
          {
            return part->GetSubcontainer(partOffset, size);
          }
        }
        return Merge().GetSubcontainer(offset, size);
      }

      static Ptr Create(FileBlocks blocks, std::size_t totalSize)
      {
        Require(!blocks.empty());
        if (blocks.size() == 1)
        {
          return blocks.begin()->second;
        }
        else
        {
          return MakePtr<ScatteredContainer>(std::move(blocks), totalSize);
        }
      }

    private:
      const Binary::Container& Merge() const
      {
        if (!Merged)
        {
          Binary::DataBuilder res(TotalSize);
          for (const auto& blk : Blocks)
          {
            res.Add(*blk.second);
          }
          Blocks.clear();
          Merged = res.CaptureResult();
        }
        return *Merged;
      }

    private:
      const std::size_t TotalSize;
      mutable FileBlocks Blocks;
      mutable Binary::Container::Ptr Merged;
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {}

      void Parse(Builder& target)
      {
        ParseDir(0, "/"_sv, target);
      }

    private:
      void ParseDir(uint_t depth, StringView path, Builder& target)
      {
        Require(depth < 10);
        const uint_t entries = Stream.Read<le_uint32_t>();
        Dbg("{} entries at '{}'", entries, path);
        for (uint_t idx = 0; idx < entries; ++idx)
        {
          const auto entryPos = Stream.GetPosition();
          const DirectoryEntry entry(Stream);
          Dbg("{} (offset={} size={} block={})", entry.Name, entry.Offset, entry.Size, entry.BlockSize);
          auto entryPath = path + entry.Name;
          Stream.Seek(entry.Offset);
          if (entry.IsDir())
          {
            Require(entryPos < entry.Offset);
            ParseDir(depth + 1, entryPath + '/', target);
          }
          else if (0 == entry.Size)
          {
            // empty file may have zero offset
            target.OnFile(entryPath, Binary::Container::Ptr());
          }
          else
          {
            Require(entryPos < entry.Offset);
            auto blocks = ParseFileBlocks(entry.Size, entry.BlockSize);
            target.OnFile(entryPath, ScatteredContainer::Create(std::move(blocks), entry.Size));
          }
          Stream.Seek(entryPos + DirectoryEntry::RAW_SIZE);
        }
      }

      FileBlocks ParseFileBlocks(std::size_t fileSize, std::size_t blockSize)
      {
        const auto blocksCount = (fileSize + blockSize - 1) / blockSize;
        std::vector<std::size_t> blocksSizes(blocksCount);
        for (auto& size : blocksSizes)
        {
          size = Stream.Read<le_uint32_t>();
        }
        FileBlocks result;
        std::size_t offset = 0;
        for (const auto size : blocksSizes)
        {
          const auto unpackedSize = std::min(blockSize, fileSize - offset);
          Dbg(" @{}: {} -> {}", offset, size, unpackedSize);
          auto packed = Stream.ReadContainer(size);
          auto unpacked = Binary::Compression::Zlib::CreateDeferredDecompressContainer(std::move(packed), unpackedSize);
          result.emplace(offset, std::move(unpacked));
          offset += unpackedSize;
        }
        Require(offset == fileSize);
        return result;
      }

    private:
      Binary::InputStream Stream;
    };

    void ParseVFS(const Binary::Container& data, Builder& target)
    {
      Format(data).Parse(target);
    }

    const auto FORMAT =
        "'P'S'F"
        "02"
        ""_sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {}

      String GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const override
      {
        return {};  // TODO
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace Playstation2SoundFormat

  Decoder::Ptr CreatePSF2Decoder()
  {
    return MakePtr<Playstation2SoundFormat::Decoder>();
  }
}  // namespace Formats::Chiptune
