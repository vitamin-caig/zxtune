/**
 *
 * @file
 *
 * @brief  RAR archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/archived/rar.h"

#include "binary/container_base.h"
#include "binary/data_builder.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "debug/log.h"
#include "formats/archived.h"
#include "math/numeric.h"
#include "strings/encoding.h"
#include "tools/xrange.h"

#include "make_ptr.h"
#include "string_view.h"

#include "3rdparty/unrar/rar.hpp"

#include <memory>

namespace Formats::Archived
{
  namespace Rar
  {
    const Debug::Stream Dbg("Formats::Archived::Rar");

    const char* UnsupportedReason(const FileBlockHeader& hdr)
    {
      const uint_t flags = hdr.Extended.Block.Flags;
      // multivolume files are not suported
      if (0 != (flags & (FileBlockHeader::FLAG_SPLIT_BEFORE | FileBlockHeader::FLAG_SPLIT_AFTER)))
      {
        return "multivolume";
      }
      // encrypted files are not supported
      if (0 != (flags & FileBlockHeader::FLAG_ENCRYPTED))
      {
        return "encrypted";
      }
      // big files are not supported
      if (hdr.IsBigFile())
      {
        return "big";
      }
      // skip directory
      if (FileBlockHeader::FLAG_DIRECTORY == (flags & FileBlockHeader::FLAG_DIRECTORY))
      {
        return "dir";
      }
      // skip empty files
      if (0 == hdr.UnpackedSize)
      {
        return "empty";
      }
      // skip invalid version
      if (!Math::InRange<uint_t>(hdr.DepackerVersion, FileBlockHeader::MIN_VERSION, FileBlockHeader::MAX_VERSION))
      {
        return "bad version";
      }
      return nullptr;
    }

    uint64_t GetBlockSize(const Binary::DataInputStream& stream)
    {
      if (const auto* block = stream.PeekField<BlockHeader>())
      {
        uint64_t res = block->Size;
        if (block->IsExtended())
        {
          if (const auto* extBlock = stream.PeekField<ExtendedBlockHeader>())
          {
            res += extBlock->AdditionalSize;
            // Even if big files are not supported, we should properly skip them in stream
            if (FileBlockHeader::TYPE == extBlock->Block.Type && FileBlockHeader::FLAG_BIG_FILE & extBlock->Block.Flags)
            {
              if (const auto* bigFile = stream.PeekField<BigFileBlockHeader>())
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

    template<class F>
    void ParseBlocks(Binary::DataInputStream& stream, F&& cb)
    {
      for (;;)
      {
        if (const auto size = GetBlockSize(stream); size <= stream.GetRestSize())
        {
          cb(stream.ReadData(size));
        }
        else
        {
          break;
        }
      }
    }

    uint16_t MakeUtf16(uint8_t high, uint8_t low)
    {
      return (uint16_t(high) << 8) | low;
    }

    String DecodeFilename(StringView ascii, Binary::DataInputStream stream)
    {
      static_assert(std::is_unsigned_v<char>);
      std::basic_string<uint16_t> result;
      result.reserve(ascii.size());
      uint_t flag = 0x10000;
      uint8_t highByte = stream.ReadByte();
      while (stream.GetRestSize())
      {
        if (flag & 0x10000)
        {
          flag = 0x100 | stream.ReadByte();
        }
        const auto val = stream.ReadByte();
        switch (((flag <<= 2) & 0x300) >> 8)
        {
        case 0:  // 8 bit ascii
          result += val;
          break;
        case 1:  // high-byte
          result += MakeUtf16(highByte, val);
          break;
        case 2:  // utf16
          result += MakeUtf16(stream.ReadByte(), val);
          break;
        case 0x3:  // ascii RLE
          if (val & 0x80)
          {
            const auto delta = stream.ReadByte();
            for (auto len = (val & 0x7f) + 2; len > 0; --len)
            {
              const auto idx = result.size();
              Require(ascii.size() > idx);
              const auto low = (ascii[idx] + delta) & 0xff;
              result += MakeUtf16(highByte, low);
            }
          }
          else
          {
            for (auto len = val + 2; len > 0; --len)
            {
              const auto idx = result.size();
              Require(ascii.size() > idx);
              result += ascii[idx];
            }
          }
          break;
        }
      }
      return Strings::Utf16ToUtf8(result);
    }

    String DecodeFilename(StringView ascii, Binary::View encoded)
    {
      try
      {
        return DecodeFilename(ascii, Binary::DataInputStream(encoded));
      }
      catch (const std::exception&)
      {
        Dbg("Failed to decode UTF filename. Use fallback.");
      }
      return Strings::ToAutoUtf8(ascii);
    }

    String GetFileName(const FileBlockHeader& header)
    {
      const auto* const self = safe_ptr_cast<const char*>(&header);
      const auto* const filename = self + (header.IsBigFile() ? sizeof(BigFileBlockHeader) : sizeof(FileBlockHeader));
      const auto* end = filename + header.NameSize;
      if (header.Extended.Block.Flags & FileBlockHeader::FLAG_UNICODE_FILENAME)
      {
        const auto* delimiter = std::find(filename, end, '\0');
        if (delimiter != end)
        {
          const auto* stream = delimiter + 1;
          return DecodeFilename(MakeStringView(filename, delimiter), Binary::View(stream, end - stream));
        }
      }
      return Strings::ToAutoUtf8(MakeStringView(filename, end));
    }

    template<class F>
    void ParseFiles(Binary::DataInputStream& stream, F&& cb)
    {
      ParseBlocks(stream, [&cb](Binary::View block) {
        if (const auto* hdr = block.As<FileBlockHeader>(); hdr && hdr->Extended.Block.Type == FileBlockHeader::TYPE)
        {
          if (const auto* reason = UnsupportedReason(*hdr))
          {
            Dbg("Skip unsupported '{}': {}", GetFileName(*hdr), reason);
          }
          else
          {
            cb(*hdr, block.SubView(hdr->Extended.Block.Size));
          }
        }
      });
    }

    struct FileReference
    {
      const FileBlockHeader& Header;
      const Binary::View Payload;

      FileReference(const FileBlockHeader& header, Binary::View payload)
        : Header(header)
        , Payload(payload)
      {}

      String GetName() const
      {
        auto name = GetFileName(Header);
        std::replace(name.begin(), name.end(), '\\', '/');
        return name;
      }
    };

    class ChainDecoder
    {
    public:
      using Ptr = std::shared_ptr<const ChainDecoder>;

      ChainDecoder(Binary::Container::Ptr data, std::vector<FileReference> files)
        : Data(std::move(data))
        , Files(std::move(files))
      {
        Dbg("Found {} files in {} bytes", Files.size(), Data->Size());
        Stream.EnableShowProgress(false);
        Decoder.Init();
      }

      auto GetCount() const
      {
        return Files.size();
      }

      auto GetName(uint_t idx) const
      {
        return Files[idx].GetName();
      }

      auto GetOutputSize(uint_t idx) const
      {
        return Files[idx].Header.UnpackedSize;
      }

      Binary::Container::Ptr Decode(uint_t idx) const
      {
        const auto& file = Files[idx];
        const auto& header = file.Header;
        const bool isSolid = header.IsSolid();
        if (isSolid && idx > 0 && LastDecoded != idx - 1)
        {
          Decode(idx - 1);
        }
        const auto& payload = file.Payload;
        const std::size_t outSize = header.UnpackedSize;
        Dbg("Depack #{} {} -> {} (solid {}, method {})", idx, payload.Size(), outSize, isSolid, uint_t(header.Method));
        if (header.IsStored())
        {
          if (outSize != payload.Size())
          {
            Dbg("Stored file mismatch");
            return {};
          }
          return Unstore(payload);
        }
        // Old format starts from 52 45 7e 5e
        const bool oldFormat = false;
        try
        {
          Binary::DataBuilder result(outSize);
          Stream.SetUnpackFromMemory(payload.As<uint8_t>(), payload.Size(), oldFormat);
          Stream.SetPackedSizeToRead(payload.Size());
          Stream.SetUnpackToMemory(static_cast<byte*>(result.Allocate(outSize)), outSize);
          Decoder.SetDestSize(outSize);
          Decoder.DoUnpack(std::max<int>(header.DepackerVersion, 15), isSolid);
          if (header.UnpackedCRC != Stream.GetUnpackedCrc())
          {
            Dbg("Crc mismatch: stored 0x{:08x}, calculated 0x{:08x}", header.UnpackedCRC, Stream.GetUnpackedCrc());
          }
          LastDecoded = idx;
          return result.CaptureResult();
        }
        catch (const std::exception& e)
        {
          Dbg("Failed to decode: {}", e.what());
          LastDecoded = NO_DECODED;
          return {};
        }
      }

    private:
      Binary::Container::Ptr Unstore(Binary::View payload) const
      {
        const auto offset = payload.As<uint8_t>() - Binary::View(*Data).As<uint8_t>();
        Require(offset + payload.Size() <= Data->Size());
        return Data->GetSubcontainer(offset, payload.Size());
      }

    private:
      static constexpr auto NO_DECODED = uint_t(-1);

      const Binary::Container::Ptr Data;
      const std::vector<FileReference> Files;
      mutable ComprDataIO Stream;
      mutable Unpack Decoder{&Stream};
      mutable uint_t LastDecoded = NO_DECODED;
    };

    class File : public Archived::File
    {
    public:
      File(ChainDecoder::Ptr decoder, uint_t idx, String name)
        : Decoder(std::move(decoder))
        , Idx(idx)
        , Name(std::move(name))
      {
        Dbg("#{}: {}", Idx, Name);
      }

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return Decoder->GetOutputSize(Idx);
      }

      Binary::Container::Ptr GetData() const override
      {
        return Decoder->Decode(Idx);
      }

    private:
      const ChainDecoder::Ptr Decoder;
      const uint_t Idx;
      const String Name;
    };

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      Container(Binary::Container::Ptr data, std::vector<FileReference> files)
        : BaseContainer(data)
        , Decoder(MakePtr<ChainDecoder>(std::move(data), std::move(files)))
      {}

      // Archive::Container
      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (auto idx : xrange(Decoder->GetCount()))
        {
          walker.OnFile(File(Decoder, idx, Decoder->GetName(idx)));
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        // TODO: fast lookup?
        for (auto idx : xrange(Decoder->GetCount()))
        {
          auto fileName = Decoder->GetName(idx);
          if (name == fileName)
          {
            return MakePtr<File>(Decoder, idx, std::move(fileName));
          }
        }
        return {};
      }

      uint_t CountFiles() const override
      {
        return Decoder->GetCount();
      }

    private:
      const ChainDecoder::Ptr Decoder;
    };

    const auto DESCRIPTION = "RAR"sv;
    const auto FORMAT =
        // file marker
        "5261"  // uint16_t CRC;   "Ra"
        "72"    // uint8_t Type;   "r"
        "211a"  // uint16_t Flags; "!ESC^"
        "0700"  // uint16_t Size;  "BELL^,0"
        // archive header
        "??"           // uint16_t CRC;
        "73"           // uint8_t Type;
        "%0xxxxxxx ?"  // uint16_t Flags; - no encrypted headers
        ""sv;
  }  // namespace Rar

  class RarDecoder : public Decoder
  {
  public:
    RarDecoder()
      : Format(Binary::CreateFormat(Rar::FORMAT))
    {}

    StringView GetDescription() const override
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

      Binary::InputStream input(data);
      std::vector<Rar::FileReference> files;
      Rar::ParseFiles(input, [&files](const auto& hdr, auto payload) { files.emplace_back(hdr, payload); });
      return MakePtr<Rar::Container>(input.GetReadContainer(), std::move(files));
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateRarDecoder()
  {
    return MakePtr<RarDecoder>();
  }
}  // namespace Formats::Archived
