/**
 *
 * @file
 *
 * @brief  TeleDisk images support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/container.h"
#include "formats/packed/image_utils.h"
#include "formats/packed/pack_utils.h"

#include "binary/compression/lha.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "debug/log.h"
#include "formats/packed.h"
#include "math/numeric.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"

#include <cstring>
#include <numeric>

namespace Formats::Packed
{
  namespace TeleDiskImage
  {
    const Debug::Stream Dbg("Formats::Packed::TeleDiskImage");

    struct RawHeader
    {
      le_uint16_t ID;
      uint8_t Sequence;
      uint8_t CheckSequence;
      uint8_t Version;
      uint8_t DataRate;
      uint8_t DriveType;
      uint8_t Stepping;
      uint8_t DOSAllocation;
      uint8_t Sides;
      le_uint16_t CRC;

      bool HasComment() const
      {
        return 0 != (Stepping & 0x80);
      }
    };

    struct RawComment
    {
      le_uint16_t CRC;
      le_uint16_t Size;
      uint8_t Year;
      uint8_t Month;
      uint8_t Day;
      uint8_t Hour;
      uint8_t Minute;
      uint8_t Second;
    };

    struct RawTrack
    {
      uint8_t Sectors;
      uint8_t Cylinder;
      uint8_t Head;
      uint8_t CRC;

      bool IsLast() const
      {
        return Sectors == 0xff;
      }
    };

    struct RawSector
    {
      uint8_t Cylinder;
      uint8_t Head;
      uint8_t Number;
      uint8_t Size;
      uint8_t Flags;
      uint8_t CRC;

      enum FlagBits
      {
        DUPLICATED = 1,
        CRC_ERROR = 2,
        DELETED = 4,
        SKIPPED = 16,
        NO_DATA = 32,
        NO_ID = 64,
      };

      bool NoData() const
      {
        return 0 != (Flags & (SKIPPED | NO_DATA));
      }

      bool NoId() const
      {
        return 0 != (Flags & NO_ID);
      }

      bool IsLast() const
      {
        return Number == 0x65;
      }
    };

    struct RawData
    {
      le_uint16_t Size;
      uint8_t Method;
    };

    struct R2PEntry
    {
      le_uint16_t Count;
      uint8_t Data[2];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 12, "Invalid layout");
    static_assert(sizeof(RawComment) * alignof(RawComment) == 10, "Invalid layout");
    static_assert(sizeof(RawTrack) * alignof(RawTrack) == 4, "Invalid layout");
    static_assert(sizeof(RawSector) * alignof(RawSector) == 6, "Invalid layout");
    static_assert(sizeof(RawData) * alignof(RawData) == 3, "Invalid layout");
    static_assert(sizeof(R2PEntry) * alignof(R2PEntry) == 4, "Invalid layout");

    const uint_t MAX_CYLINDERS_COUNT = 100;
    const uint_t MIN_SIDES_COUNT = 1;
    const uint_t MAX_SIDES_COUNT = 2;
    const std::size_t MAX_SECTOR_SIZE = 8192;

    const uint_t ID_OLD = 0x4454;
    const uint_t ID_NEW = 0x6474;

    const std::size_t MIN_SIZE = sizeof(RawHeader);
    const std::size_t MAX_IMAGE_SIZE = 1048576;
    const String COMPRESSION_ALGORITHM("-lh1-");

    enum SectorDataType
    {
      RAW_SECTOR = 0,
      R2P_SECTOR,
      RLE_SECTOR
    };

    class ImageVisitor
    {
    public:
      virtual ~ImageVisitor() = default;

      virtual void OnSector(const Formats::CHS& loc, Binary::Data::Ptr data, SectorDataType type,
                            std::size_t targetSize) = 0;
    };

    auto DecodeR2P(Binary::View data)
    {
      Require(data.Size() % sizeof(R2PEntry) == 0);
      Binary::DataBuilder tmp(MAX_SECTOR_SIZE);
      for (const R2PEntry *it = data.As<R2PEntry>(), *lim = it + data.Size() / sizeof(*it); it != lim; ++it)
      {
        const uint_t count = it->Count;
        Require(count != 0);
        tmp.AddByte(it->Data[0]);
        tmp.AddByte(it->Data[1]);
        Require(CopyFromBack(sizeof(it->Data), tmp, sizeof(it->Data) * (count - 1)));
      }
      return tmp.CaptureResult();
    }

    auto DecodeRLE(Binary::View data)
    {
      Binary::DataBuilder tmp(MAX_SECTOR_SIZE);
      ByteStream stream(data.As<uint8_t>(), data.Size());
      while (!stream.Eof())
      {
        const uint_t len = 2 * stream.GetByte();
        Require(!stream.Eof());
        const uint_t count = stream.GetByte();
        Require(count != 0);

        const bool isRLE = len != 0;
        const uint_t blockSize = isRLE ? len : count;
        Require(stream.GetRestBytes() >= blockSize);
        for (uint_t idx = 0; idx != blockSize; ++idx)
        {
          tmp.AddByte(stream.GetByte());
        }
        if (isRLE)
        {
          Require(CopyFromBack(len, tmp, len * (count - 1)));
        }
      }
      return tmp.CaptureResult();
    }

    class ImageVisitorAdapter : public ImageVisitor
    {
    public:
      explicit ImageVisitorAdapter(Formats::ImageBuilder& builder)
        : Builder(builder)
      {}

      void OnSector(const Formats::CHS& loc, Binary::Data::Ptr data, SectorDataType type,
                    std::size_t targetSize) override
      {
        switch (type)
        {
        case RAW_SECTOR:
          Sectors.emplace_back(std::move(data));
          break;
        case R2P_SECTOR:
          Sectors.emplace_back(DecodeR2P(*data));
          break;
        case RLE_SECTOR:
          Sectors.emplace_back(DecodeRLE(*data));
          break;
        }
        Builder.SetSector(loc, *Sectors.back());
        Require(targetSize == Sectors.back()->Size());
      }

    private:
      Formats::ImageBuilder& Builder;
      std::vector<Binary::Data::Ptr> Sectors;
    };

    void ParseSectors(Binary::InputStream& stream, bool hasComment, ImageVisitor& visitor)
    {
      if (hasComment)
      {
        const auto& comment = stream.Read<RawComment>();
        Dbg("Created at: {}-{}-{} {}:{}:{}", 1900 + comment.Year, comment.Month, comment.Day, comment.Hour,
            comment.Minute, comment.Second);
        if (const std::size_t size = comment.Size)
        {
          Binary::DataInputStream strings(stream.ReadData(size));
          while (const auto rest = strings.GetRestSize())
          {
            Dbg("> {}", strings.ReadCString(rest));
          }
        }
      }

      for (;;)
      {
        const auto& track = stream.Read<RawTrack>();
        if (track.IsLast())
        {
          break;
        }
        Require(Math::InRange<uint_t>(track.Cylinder, 0, MAX_CYLINDERS_COUNT));
        for (uint_t sect = 0; sect != track.Sectors; ++sect)
        {
          const auto& sector = stream.Read<RawSector>();
          if (sector.NoData())
          {
            continue;
          }
          else if (sector.IsLast())
          {
            break;
          }
          Require(Math::InRange<uint_t>(sector.Size, 0, 6));
          const std::size_t sectorSize = std::size_t(128) << sector.Size;
          const auto& srcDataDesc = stream.Read<RawData>();
          Require(Math::InRange<uint_t>(srcDataDesc.Method, RAW_SECTOR, RLE_SECTOR));
          Require(srcDataDesc.Size > 1);
          auto sectorData = stream.ReadContainer(srcDataDesc.Size - 1);
          const Formats::CHS loc(sector.Cylinder, track.Head, sector.Number);
          // use track parameters for layout
          if (sector.NoId() || sector.Number > track.Sectors)
          {
            Dbg("Skip sector {}:{}:{}", loc.Cylinder, loc.Head, loc.Sector);
          }
          else
          {
            visitor.OnSector(loc, std::move(sectorData), static_cast<SectorDataType>(srcDataDesc.Method), sectorSize);
          }
        }
      }
    }

    std::size_t Parse(const Binary::Container& rawData, ImageVisitor& visitor)
    {
      Binary::InputStream stream(rawData);
      try
      {
        const auto& header = stream.Read<RawHeader>();
        const uint_t id = header.ID;
        Require(id == ID_OLD || id == ID_NEW);
        Require(header.Sequence == 0);
        Require(Math::InRange<uint_t>(header.Sides, MIN_SIDES_COUNT, MAX_SIDES_COUNT));
        const bool compressedData = id == ID_NEW;
        const bool newCompression = header.Version > 20;
        if (compressedData)
        {
          if (!newCompression)
          {
            Dbg("Old compression is not supported.");
            return 0;
          }
          const auto packedPosition = stream.GetPosition();
          if (const auto fullDecoded =
                  Binary::Compression::Lha::DecodeRawData(stream, COMPRESSION_ALGORITHM, MAX_IMAGE_SIZE))
          {
            Binary::InputStream subStream(*fullDecoded);
            ParseSectors(subStream, header.HasComment(), visitor);
            const std::size_t usedInPacked = subStream.GetPosition();
            Dbg("Used {}/{} bytes in packed stream", usedInPacked, fullDecoded->Size());
            stream.Seek(packedPosition);
            if (const auto decoded =
                    Binary::Compression::Lha::DecodeRawData(stream, COMPRESSION_ALGORITHM, usedInPacked))
            {
              const std::size_t usedSize = stream.GetPosition() - packedPosition;
              Dbg("Used {}/{} bytes in source stream", usedSize, usedSize + stream.GetRestSize());
              return sizeof(header) + usedSize;
            }
          }
          Dbg("Failed to decode lha stream");
          return 0;
        }
        else
        {
          ParseSectors(stream, header.HasComment(), visitor);
        }
        return stream.GetPosition();
      }
      catch (const std::exception&)
      {
        Dbg("Failed to parse");
        return 0;
      }
    }

    const auto DESCRIPTION = "TD0 (TeleDisk Image)"sv;
    const auto FORMAT_PATTERN =
        "('T|'t)('D|'d)"  // uint8_t ID[2]
        "00"              // uint8_t Sequence;
        "?"               // uint8_t CheckSequence;
        "?"               // uint8_t Version;
        "%x00000xx"       // uint8_t DataRate;
        "00-06"           // uint8_t DriveType;
        "%x00000xx"       // uint8_t Stepping;
                          /*
                          "?"           // uint8_t DOSAllocation;
                          "?"           // uint8_t Sides;
                          "??"          // uint16_t CRC;
                          */
        ""sv;
  }  // namespace TeleDiskImage

  class TeleDiskImageDecoder : public Decoder
  {
  public:
    TeleDiskImageDecoder()
      : Format(Binary::CreateFormat(TeleDiskImage::FORMAT_PATTERN, TeleDiskImage::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return TeleDiskImage::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Format->Match(rawData))
      {
        return {};
      }
      const auto builder = CreateSparsedImageBuilder();
      TeleDiskImage::ImageVisitorAdapter visitor(*builder);
      if (const std::size_t usedSize = TeleDiskImage::Parse(rawData, visitor))
      {
        return CreateContainer(builder->GetResult(), usedSize);
      }
      return {};
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateTeleDiskImageDecoder()
  {
    return MakePtr<TeleDiskImageDecoder>();
  }
}  // namespace Formats::Packed
