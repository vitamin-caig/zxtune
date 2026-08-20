/**
 *
 * @file
 *
 * @brief  ZIP archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/archived/zip.h"

#include "binary/compression/zlib_stream.h"
#include "binary/container_base.h"
#include "binary/data_builder.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "debug/log.h"
#include "formats/archived.h"
#include "strings/encoding.h"
#include "strings/map.h"

#include "make_ptr.h"
#include "string_view.h"

#include <optional>

namespace Formats::Archived
{
  namespace Zip
  {
    const Debug::Stream Dbg("Formats::Archived::Zip");

    template<class T>
    const T* GetBlock(Binary::View data)
    {
      if (const auto* raw = data.As<T>(); raw && raw->Signature == T::SIGNATURE)
      {
        return raw;
      }
      return nullptr;
    }

    std::optional<std::size_t> FindFooter(Binary::View data)
    {
      const decltype(LocalFileFooter::Signature) signature = LocalFileFooter::SIGNATURE;
      const auto* const rawSignature = safe_ptr_cast<const uint8_t*>(&signature);

      const auto* const seekStart = data.As<uint8_t>();
      const auto* const seekEnd = seekStart + data.Size();
      for (const auto* seekPos = seekStart; seekPos < seekEnd;)
      {
        const auto* const found = std::search(seekPos, seekEnd, rawSignature, rawSignature + sizeof(signature));
        if (found == seekEnd)
        {
          break;
        }
        const auto offset = found - seekStart;
        if (offset + sizeof(LocalFileFooter) > data.Size())
        {
          break;
        }
        const auto& result = *safe_ptr_cast<const LocalFileFooter*>(found);
        if (offset == result.Attributes.CompressedSize)
        {
          return offset;
        }
        seekPos = found + sizeof(signature);
      }
      return std::nullopt;
    }

    std::size_t GetBlockSize(Binary::View data)
    {
      if (const auto* header = GetBlock<LocalFileHeader>(data))
      {
        const auto headerSize = header->GetSize();
        if (header->NeedFooter())
        {
          if (const auto offset = FindFooter(data.SubView(headerSize)))
          {
            return headerSize + *offset;
          }
        }
        else
        {
          return headerSize + header->Attributes.CompressedSize;
        }
      }
      else if (const auto* footer = GetBlock<LocalFileFooter>(data))
      {
        return sizeof(*footer);
      }
      else if (const auto* extra = GetBlock<ExtraDataRecord>(data))
      {
        return extra->GetSize();
      }
      else if (const auto* centralHeader = GetBlock<CentralDirectoryFileHeader>(data))
      {
        return centralHeader->GetSize();
      }
      else if (const auto* centralFooter = GetBlock<CentralDirectoryEnd>(data))
      {
        return centralFooter->GetSize();
      }
      else if (const auto* signature = GetBlock<DigitalSignature>(data))
      {
        return signature->GetSize();
      }
      else
      {
        Dbg("Unknown block");
      }
      return 0;
    }

    template<class F>
    std::size_t ParseBlocks(Binary::View data, F&& cb)
    {
      std::size_t consumed = 0;
      // Binary::View cannot be reassigned directly
      std::optional<Binary::View> source{data};
      while (*source)
      {
        if (const auto size = GetBlockSize(*source); size && size <= source->Size())
        {
          cb(source->SubView(0, size));
          consumed += size;
          source.emplace(source->SubView(size));
        }
        else
        {
          break;
        }
      }
      return consumed;
    }

    const char* UnsupportedReason(const LocalFileHeader& header, std::size_t unpackedSize)
    {
      if (header.Flags & FILE_CRYPTED)
      {
        return "encrypted";
      }
      if (header.CompressionMethod != 0 && header.CompressionMethod != 8 && header.CompressionMethod != 9)
      {
        return "unsupported compression method";
      }
      if (!unpackedSize)
      {
        return "empty";
      }
      return nullptr;
    }

    String GetFileName(const LocalFileHeader& header)
    {
      // See ${modland}/Renoise/Farmer/funktastrophe.xrns
      const char* start = header.Name;
      const char* end = std::find(start, start + header.NameSize, 0);
      const auto rawName = MakeStringView(start, end);
      const bool isUtf8 = 0 != (header.Flags & FILE_UTF8);
      return isUtf8 ? String{rawName} : Strings::ToAutoUtf8(rawName);
    }

    template<class F>
    auto ParseFiles(Binary::View data, F&& cb)
    {
      std::optional<Binary::View> delayedFile;
      return ParseBlocks(data, [&cb, &delayedFile](Binary::View block) {
        const auto* header = GetBlock<LocalFileHeader>(block);
        if (header && header->NeedFooter())
        {
          // postpone
          delayedFile.emplace(block);
          return;
        }
        const LocalFileFooter* footer = header ? nullptr : GetBlock<LocalFileFooter>(block);
        if (footer && !delayedFile)
        {
          Dbg("Ignored LocalFileFooter");
          return;
        }
        if (header || footer)
        {
          // TODO: think about more clean solution
          const auto& hdr = header ? *header : *delayedFile->As<LocalFileHeader>();
          const auto payload = header ? block.SubView(header->GetSize()) : delayedFile->SubView(hdr.GetSize());
          const auto unpackedSize = footer ? footer->Attributes.UncompressedSize : header->Attributes.UncompressedSize;
          if (const auto* reason = UnsupportedReason(hdr, unpackedSize))
          {
            Dbg("Skip unsupported '{}': {}", GetFileName(hdr), reason);
          }
          else
          {
            cb(hdr, payload, unpackedSize);
          }
        }
        delayedFile.reset();
      });
    }

    struct FileReference
    {
      const LocalFileHeader& Header;
      const Binary::View Payload;
      const std::size_t UnpackedSize;

      FileReference(const LocalFileHeader& header, Binary::View payload, std::size_t unpackedSize)
        : Header(header)
        , Payload(payload)
        , UnpackedSize(unpackedSize)
      {}

      String GetName() const
      {
        return GetFileName(Header);
      }

      Binary::Container::Ptr Decompress(const Binary::Container& data) const
      {
        Dbg("Depack '{}' {} -> {} (method {})", GetName(), Payload.Size(), UnpackedSize,
            uint_t(Header.CompressionMethod));
        if (Header.CompressionMethod != 0)
        {
          return Decompress();
        }
        else
        {
          return Destore(data);
        }
      }

    private:
      Binary::Container::Ptr Decompress() const
      {
        Binary::DataInputStream input(Payload);
        Binary::DataBuilder output(UnpackedSize);
        Binary::Compression::Zlib::DecompressRaw(input, output, UnpackedSize);
        if (UnpackedSize != output.Size())
        {
          Dbg("Unpacked size mismatch: {} actually", output.Size());
          return {};
        }
        return output.CaptureResult();
      }

      Binary::Container::Ptr Destore(const Binary::Container& data) const
      {
        if (UnpackedSize != Payload.Size())
        {
          Dbg("Unpacked size mismatch: {} actually", Payload.Size());
          return {};
        }
        const auto offset = Payload.As<uint8_t>() - Binary::View(data).As<uint8_t>();
        Require(offset + Payload.Size() <= data.Size());
        return data.GetSubcontainer(offset, Payload.Size());
      }
    };

    class File : public Archived::File
    {
    public:
      File(Binary::Container::Ptr data, const FileReference& ref, String name)
        : Data(std::move(data))
        , Ref(std::move(ref))
        , Name(name)
      {}

      String GetName() const override
      {
        return Name;
      }

      std::size_t GetSize() const override
      {
        return Ref.UnpackedSize;
      }

      Binary::Container::Ptr GetData() const override
      {
        return Ref.Decompress(*Data);
      }

    private:
      const Binary::Container::Ptr Data;
      const FileReference Ref;
      const String Name;
    };

    class Container : public Binary::BaseContainer<Archived::Container>
    {
    public:
      Container(Binary::Container::Ptr data, std::vector<FileReference> files)
        : BaseContainer(std::move(data))
        , Files(std::move(files))
      {
        Dbg("Found {} files in {} bytes", Files.size(), Delegate->Size());
      }

      void ExploreFiles(const Container::Walker& walker) const override
      {
        for (const auto& f : Files)
        {
          walker.OnFile(File(Delegate, f, f.GetName()));
        }
      }

      File::Ptr FindFile(StringView name) const override
      {
        for (const auto& f : Files)
        {
          auto fileName = f.GetName();
          if (name == fileName)
          {
            return MakePtr<File>(Delegate, f, std::move(fileName));
          }
        }
        return {};
      }

      uint_t CountFiles() const override
      {
        return Files.size();
      }

    private:
      const std::vector<FileReference> Files;
    };

    const auto DESCRIPTION = "ZIP"sv;
    const auto FORMAT =
        "504b0304"             // uint32_t Signature;
        "?00"                  // uint16_t VersionToExtract;
        "%0000xxx0 %0000x000"  // uint16_t Flags;
        "%0000x00x 00"         // uint16_t CompressionMethod;
        ""sv;
  }  // namespace Zip

  class ZipDecoder : public Decoder
  {
  public:
    ZipDecoder()
      : Format(Binary::CreateFormat(Zip::FORMAT))
    {}

    StringView GetDescription() const override
    {
      return Zip::DESCRIPTION;
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
      std::vector<Zip::FileReference> files;
      const auto totalSize = Zip::ParseFiles(data, [&files](const auto& hdr, auto payload, auto unpackedSize) {
        files.emplace_back(hdr, payload, unpackedSize);
      });
      return totalSize ? MakePtr<Zip::Container>(data.GetSubcontainer(0, totalSize), std::move(files))
                       : Container::Ptr();
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateZipDecoder()
  {
    return MakePtr<ZipDecoder>();
  }
}  // namespace Formats::Archived
