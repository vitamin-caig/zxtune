/**
 *
 * @file
 *
 * @brief  RAR compressor support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
#include "formats/packed/pack_utils.h"
#include "formats/packed/rar_supp.h"
// common includes
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/packed.h>
#include <math/numeric.h>
// std includes
#include <array>
#include <cassert>
#include <limits>
#include <memory>
// thirdparty
#include <3rdparty/unrar/rar.hpp>

#undef min
#undef max

namespace Formats::Packed
{
  namespace Rar
  {
    const Debug::Stream Dbg("Formats::Packed::Rar");

    const Char DESCRIPTION[] = "RAR";
    const auto HEADER_PATTERN =
        "??"          // uint16_t CRC;
        "74"          // uint8_t Type;
        "?%1xxxxxxx"  // uint16_t Flags;
        ""_sv;

    class Container
    {
    public:
      explicit Container(const Binary::Container& data)
        : Data(data)
      {}

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
        if (const auto* header = GetHeader<FileBlockHeader>())
        {
          if (!header->IsValid() || !header->IsSupported())
          {
            return 0;
          }
          uint64_t res = header->Extended.Block.Size;
          res += header->Extended.AdditionalSize;
          if (header->IsBigFile())
          {
            if (const auto* bigHeader = GetHeader<BigFileBlockHeader>())
            {
              res += uint64_t(bigHeader->PackedSizeHi) << (8 * sizeof(uint32_t));
            }
            else
            {
              return 0;
            }
          }
          const std::size_t maximum = std::numeric_limits<std::size_t>::max();
          return res > maximum ? maximum : static_cast<std::size_t>(res);
        }
        return false;
      }

      const Formats::Packed::Rar::FileBlockHeader& GetHeader() const
      {
        return *GetHeader<FileBlockHeader>();
      }

      Binary::Container::Ptr GetData() const
      {
        const auto& header = GetHeader();
        const std::size_t offset = header.Extended.Block.Size;
        const std::size_t size = header.Extended.AdditionalSize;
        return Data.GetSubcontainer(offset, size);
      }

    private:
      template<class T>
      const T* GetHeader() const
      {
        return Binary::View(Data).As<T>();
      }

    private:
      const Binary::Container& Data;
    };

    class CompressedFile
    {
    public:
      using Ptr = std::unique_ptr<const CompressedFile>;
      virtual ~CompressedFile() = default;

      virtual Binary::Container::Ptr Decompress(const Container& container) const = 0;
    };

    class StoredFile : public CompressedFile
    {
    public:
      Binary::Container::Ptr Decompress(const Container& container) const override
      {
        const auto& header = container.GetHeader();
        assert(0x30 == header.Method);
        const std::size_t outSize = header.UnpackedSize;
        auto data = container.GetData();
        if (data->Size() != outSize)
        {
          Dbg("Stored file sizes mismatch");
          return Binary::Container::Ptr();
        }
        else
        {
          Dbg("Restore");
          return data;
        }
      }
    };

    class PackedFile : public CompressedFile
    {
    public:
      PackedFile()
        : Decoder(&Stream)
      {
        Stream.EnableShowProgress(false);
        Decoder.Init();
      }

      Binary::Container::Ptr Decompress(const Container& container) const override
      {
        const auto& header = container.GetHeader();
        assert(0x30 != header.Method);
        const std::size_t outSize = header.UnpackedSize;
        const bool isSolid = header.IsSolid();
        const auto data = container.GetData();
        const auto size = data->Size();
        Dbg("Depack {} -> {} (solid {})", size, outSize, isSolid);
        // Old format starts from 52 45 7e 5e
        const bool oldFormat = false;
        Stream.SetUnpackFromMemory(static_cast<const uint8_t*>(data->Start()), size, oldFormat);
        Stream.SetPackedSizeToRead(size);
        const int method = std::max<int>(header.DepackerVersion, 15);
        return Decompress(method, outSize, isSolid, header.UnpackedCRC);
      }

    private:
      Binary::Container::Ptr Decompress(int method, std::size_t outSize, bool isSolid, uint32_t crc) const
      {
        try
        {
          Binary::DataBuilder result(outSize);
          Stream.SetUnpackToMemory(static_cast<byte*>(result.Allocate(outSize)), outSize);
          Decoder.SetDestSize(outSize);
          Decoder.DoUnpack(method, isSolid);
          if (crc != Stream.GetUnpackedCrc())
          {
            Dbg("Crc mismatch: stored 0x{:08x}, calculated 0x{:08x}", crc, Stream.GetUnpackedCrc());
          }
          return result.CaptureResult();
        }
        catch (const std::exception& e)
        {
          Dbg("Failed to decode: {}", e.what());
          return Binary::Container::Ptr();
        }
      }

    private:
      mutable ComprDataIO Stream;
      mutable Unpack Decoder;
    };

    class DispatchedCompressedFile : public CompressedFile
    {
    public:
      DispatchedCompressedFile()
        : Packed()
        , Stored()
      {}

      Binary::Container::Ptr Decompress(const Container& container) const override
      {
        const auto& header = container.GetHeader();
        if (header.IsStored())
        {
          return Stored.Decompress(container);
        }
        else
        {
          return Packed.Decompress(container);
        }
      }

    private:
      const PackedFile Packed;
      const StoredFile Stored;
    };

    String FileBlockHeader::GetName() const
    {
      const auto* const self = safe_ptr_cast<const uint8_t*>(this);
      const uint8_t* const filename = self + (IsBigFile() ? sizeof(BigFileBlockHeader) : sizeof(FileBlockHeader));
      return String(filename, filename + NameSize);
    }

    bool FileBlockHeader::IsValid() const
    {
      return Extended.Block.IsExtended() && Extended.Block.Type == TYPE;
    }

    bool FileBlockHeader::IsSupported() const
    {
      const uint_t flags = Extended.Block.Flags;
      // multivolume files are not suported
      if (0 != (flags & (FileBlockHeader::FLAG_SPLIT_BEFORE | FileBlockHeader::FLAG_SPLIT_AFTER)))
      {
        return false;
      }
      // encrypted files are not supported
      if (0 != (flags & FileBlockHeader::FLAG_ENCRYPTED))
      {
        return false;
      }
      // big files are not supported
      if (IsBigFile())
      {
        return false;
      }
      // skip directory
      if (FileBlockHeader::FLAG_DIRECTORY == (flags & FileBlockHeader::FLAG_DIRECTORY))
      {
        return false;
      }
      // skip empty files
      if (0 == UnpackedSize)
      {
        return false;
      }
      // skip invalid version
      if (!Math::InRange<uint_t>(DepackerVersion, MIN_VERSION, MAX_VERSION))
      {
        return false;
      }
      return true;
    }
  }  // namespace Rar

  class RarDecoder : public Decoder
  {
  public:
    RarDecoder()
      : Format(Binary::CreateFormat(Rar::HEADER_PATTERN))
      , Decoder(MakePtr<Rar::DispatchedCompressedFile>())
    {}

    String GetDescription() const override
    {
      return Rar::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      const Rar::Container container(rawData);
      if (!container.FastCheck())
      {
        return Container::Ptr();
      }
      return CreateContainer(Decoder->Decompress(container), container.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Format;
    const Rar::CompressedFile::Ptr Decoder;
  };

  Decoder::Ptr CreateRarDecoder()
  {
    return MakePtr<RarDecoder>();
  }
}  // namespace Formats::Packed
