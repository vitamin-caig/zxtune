/**
 *
 * @file
 *
 * @brief  ZIP compressor support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
#include "formats/packed/zip_supp.h"
// common includes
#include <error.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/compression/zlib_stream.h>
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/packed.h>
// std includes
#include <algorithm>
#include <cassert>
#include <memory>

namespace Formats::Packed
{
  namespace Zip
  {
    const Debug::Stream Dbg("Formats::Packed::Zip");

    const Char DESCRIPTION[] = "ZIP";
    const auto HEADER_PATTERN =
        "504b0304"             // uint32_t Signature;
        "?00"                  // uint16_t VersionToExtract;
        "%0000xxx0 %0000x000"  // uint16_t Flags;
        "%0000x00x 00"         // uint16_t CompressionMethod;
        ""sv;

    class Container
    {
    public:
      explicit Container(const Binary::Container& data)
        : Data(data)
        , View(data)
      {}

      bool FastCheck() const
      {
        if (View.Size() < sizeof(LocalFileHeader))
        {
          return false;
        }
        const LocalFileHeader& header = GetHeader();
        if (!header.IsValid() || !header.IsSupported())
        {
          return false;
        }
        File = CompressedFile::Create(header, View.Size());
        if (File.get() && File->GetUnpackedSize())
        {
          return File->GetPackedSize() <= View.Size();
        }
        return false;
      }

      const LocalFileHeader& GetHeader() const
      {
        assert(View.Size() >= sizeof(LocalFileHeader));
        return *View.As<LocalFileHeader>();
      }

      const CompressedFile& GetFile() const
      {
        return *File;
      }

      Binary::Container::Ptr GetPackedData() const
      {
        return Data.GetSubcontainer(GetHeader().GetSize(), File->GetPackedSize());
      }

    private:
      const Binary::Container& Data;
      const Binary::View View;
      mutable std::unique_ptr<const CompressedFile> File;
    };

    class DataDecoder
    {
    public:
      virtual ~DataDecoder() = default;

      virtual Binary::Container::Ptr Decompress() const = 0;
    };

    class StoreDataDecoder : public DataDecoder
    {
    public:
      StoreDataDecoder(Binary::Container::Ptr data, std::size_t destSize)
        : Data(std::move(data))
        , DestSize(destSize)
      {}

      Binary::Container::Ptr Decompress() const override
      {
        if (Data->Size() != DestSize)
        {
          Dbg("Stored file sizes mismatch");
          return {};
        }
        else
        {
          Dbg("Restore {} bytes", DestSize);
          return Data;
        }
      }

    private:
      const Binary::Container::Ptr Data;
      const std::size_t DestSize;
    };

    class InflatedDataDecoder : public DataDecoder
    {
    public:
      InflatedDataDecoder(Binary::Container::Ptr data, std::size_t destSize)
        : Data(std::move(data))
        , DestSize(destSize)
      {}

      Binary::Container::Ptr Decompress() const override
      {
        Dbg("Inflate {} -> {}", Data->Size(), DestSize);
        try
        {
          Binary::DataInputStream input(*Data);
          Binary::DataBuilder output(DestSize);
          Binary::Compression::Zlib::DecompressRaw(input, output, DestSize);
          Require(output.Size() == DestSize);
          return output.CaptureResult();
        }
        catch (const Error& e)
        {
          Dbg("Failed to inflate: {}", e.ToString());
        }
        catch (const std::exception&)
        {
          Dbg("Failed to inflate");
        }
        return {};
      }

    private:
      const Binary::Container::Ptr Data;
      const std::size_t DestSize;
    };

    std::unique_ptr<DataDecoder> CreateDecoder(const Container& container)
    {
      const auto& header = container.GetHeader();
      const auto& file = container.GetFile();
      auto input = container.GetPackedData();
      if (!input)
      {
        return {};
      }
      const std::size_t outSize = file.GetUnpackedSize();
      switch (header.CompressionMethod)
      {
      case 0:
        return std::unique_ptr<DataDecoder>(new StoreDataDecoder(std::move(input), outSize));
        break;
      case 8:
      case 9:
        return std::unique_ptr<DataDecoder>(new InflatedDataDecoder(std::move(input), outSize));
        break;
      }
      return {};
    }

    class DispatchedDataDecoder : public DataDecoder
    {
    public:
      explicit DispatchedDataDecoder(const Container& container)
        : Delegate(CreateDecoder(container))
        , IsValid(Delegate.get())
      {}

      Binary::Container::Ptr Decompress() const override
      {
        if (!IsValid)
        {
          return {};
        }
        auto result = Delegate->Decompress();
        IsValid = !!result;
        return result;
      }

    private:
      const std::unique_ptr<DataDecoder> Delegate;
      mutable bool IsValid;
    };

    class RegularFile : public CompressedFile
    {
    public:
      explicit RegularFile(const LocalFileHeader& header)
        : Header(header)
      {}

      std::size_t GetPackedSize() const override
      {
        return Header.GetSize() + Header.Attributes.CompressedSize;
      }

      std::size_t GetUnpackedSize() const override
      {
        return Header.Attributes.UncompressedSize;
      }

    private:
      const LocalFileHeader& Header;
    };

    class StreamedFile : public CompressedFile
    {
    public:
      StreamedFile(const LocalFileHeader& header, const LocalFileFooter& footer)
        : Header(header)
        , Footer(footer)
      {}

      std::size_t GetPackedSize() const override
      {
        return Header.GetSize() + Footer.Attributes.CompressedSize + sizeof(Footer);
      }

      std::size_t GetUnpackedSize() const override
      {
        return Footer.Attributes.UncompressedSize;
      }

    private:
      const LocalFileHeader& Header;
      const LocalFileFooter& Footer;
    };

    const LocalFileFooter* FindFooter(const LocalFileHeader& header, std::size_t size)
    {
      assert(0 != (header.Flags & FILE_ATTRIBUTES_IN_FOOTER));

      const uint32_t signature = LocalFileFooter::SIGNATURE;
      const auto* const rawSignature = safe_ptr_cast<const uint8_t*>(&signature);

      const auto* const seekStart = safe_ptr_cast<const uint8_t*>(&header);
      const uint8_t* const seekEnd = seekStart + size;
      for (const uint8_t* seekPos = seekStart; seekPos < seekEnd;)
      {
        const uint8_t* const found = std::search(seekPos, seekEnd, rawSignature, rawSignature + sizeof(signature));
        if (found == seekEnd)
        {
          return nullptr;
        }
        const std::size_t offset = found - seekStart;
        if (offset + sizeof(LocalFileFooter) > size)
        {
          return nullptr;
        }
        const LocalFileFooter& result = *safe_ptr_cast<const LocalFileFooter*>(found);
        if (result.Attributes.CompressedSize + header.GetSize() == offset)
        {
          return &result;
        }
        seekPos = found + sizeof(signature);
      }
      return nullptr;
    }

    std::unique_ptr<const CompressedFile> CompressedFile::Create(const LocalFileHeader& hdr, std::size_t availSize)
    {
      assert(availSize > sizeof(hdr));
      if (0 != (hdr.Flags & FILE_ATTRIBUTES_IN_FOOTER))
      {
        if (const LocalFileFooter* footer = FindFooter(hdr, availSize))
        {
          return std::unique_ptr<const CompressedFile>(new StreamedFile(hdr, *footer));
        }
        return {};
      }
      else
      {
        return std::unique_ptr<const CompressedFile>(new RegularFile(hdr));
      }
    }
  }  // namespace Zip

  class ZipDecoder : public Decoder
  {
  public:
    ZipDecoder()
      : Depacker(Binary::CreateFormat(Zip::HEADER_PATTERN, sizeof(Zip::LocalFileHeader)))
    {}

    String GetDescription() const override
    {
      return Zip::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Depacker;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Depacker->Match(rawData))
      {
        return {};
      }
      const Zip::Container container(rawData);
      if (!container.FastCheck())
      {
        return {};
      }
      const Zip::DispatchedDataDecoder decoder(container);
      return CreateContainer(decoder.Decompress(), container.GetFile().GetPackedSize());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateZipDecoder()
  {
    return MakePtr<ZipDecoder>();
  }
}  // namespace Formats::Packed
