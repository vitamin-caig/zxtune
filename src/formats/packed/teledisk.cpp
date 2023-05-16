/**
 *
 * @file
 *
 * @brief  TeleDisk images support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
#include "formats/packed/image_utils.h"
#include "formats/packed/pack_utils.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/packed.h>
#include <formats/packed/lha_supp.h>
#include <math/numeric.h>
// std includes
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

      virtual void OnSector(const Formats::CHS& loc, Binary::View data, SectorDataType type,
                            std::size_t targetSize) = 0;
    };

    class StubImageVisitor : public ImageVisitor
    {
    public:
      void OnSector(const Formats::CHS& /*loc*/, Binary::View data, SectorDataType type,
                    std::size_t targetSize) override
      {
        switch (type)
        {
        case RAW_SECTOR:
          Require(data.Size() == targetSize);
          break;
        case R2P_SECTOR:
          Require(data.Size() % sizeof(R2PEntry) == 0);
          break;
        default:
          break;
        }
      }
    };

    Binary::Container::Ptr DecodeR2P(Binary::View data)
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

    Binary::Container::Ptr DecodeRLE(Binary::View data)
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
      explicit ImageVisitorAdapter(Formats::ImageBuilder::Ptr builder)
        : Builder(std::move(builder))
      {}

      void OnSector(const Formats::CHS& loc, Binary::View data, SectorDataType type, std::size_t targetSize) override
      {
        switch (type)
        {
        case RAW_SECTOR:
          Require(targetSize == data.Size());
          Builder->SetSector(loc, data);
          break;
        case R2P_SECTOR:
          Sectors.push_back(DecodeR2P(data));
          Require(targetSize == Sectors.back()->Size());
          Builder->SetSector(loc, *Sectors.back());
          break;
        case RLE_SECTOR:
          Sectors.push_back(DecodeRLE(data));
          Require(targetSize == Sectors.back()->Size());
          Builder->SetSector(loc, *Sectors.back());
          break;
        }
      }

    private:
      const Formats::ImageBuilder::Ptr Builder;
      std::vector<Binary::Container::Ptr> Sectors;
    };

    class SourceStream
    {
    public:
      explicit SourceStream(Binary::View rawData)
        : Stream(rawData)
      {}

      template<class T>
      const T& Get()
      {
        return Stream.Read<T>();
      }

      const uint8_t* GetData(std::size_t size)
      {
        Require(size != 0);
        return Stream.ReadData(size).As<uint8_t>();
      }

      std::size_t GetOffset() const
      {
        return Stream.GetPosition();
      }

    private:
      Binary::DataInputStream Stream;
    };

    void ParseSectors(SourceStream& stream, ImageVisitor& visitor)
    {
      for (;;)
      {
        const auto& track = stream.Get<RawTrack>();
        if (track.IsLast())
        {
          break;
        }
        Require(Math::InRange<uint_t>(track.Cylinder, 0, MAX_CYLINDERS_COUNT));
        for (uint_t sect = 0; sect != track.Sectors; ++sect)
        {
          const auto& sector = stream.Get<RawSector>();
          if (sector.NoData())
          {
            continue;
          }
          Require(Math::InRange<uint_t>(sector.Size, 0, 6));
          const std::size_t sectorSize = std::size_t(128) << sector.Size;
          const auto& srcDataDesc = stream.Get<RawData>();
          Require(Math::InRange<uint_t>(srcDataDesc.Method, RAW_SECTOR, RLE_SECTOR));
          const std::size_t dataSize = srcDataDesc.Size - 1;
          const uint8_t* const rawData = stream.GetData(dataSize);
          // use track parameters for layout
          if (!sector.NoId())
          {
            const Formats::CHS loc(sector.Cylinder, track.Head, sector.Number);
            visitor.OnSector(loc, {rawData, dataSize}, static_cast<SectorDataType>(srcDataDesc.Method), sectorSize);
          }
        }
      }
    }

    std::size_t Parse(const Binary::Container& rawData, ImageVisitor& visitor)
    {
      SourceStream stream(rawData);
      try
      {
        const auto& header = stream.Get<RawHeader>();
        const uint_t id = header.ID;
        Require(id == ID_OLD || id == ID_NEW);
        Require(header.Sequence == 0);
        Require(Math::InRange<uint_t>(header.Sides, MIN_SIDES_COUNT, MAX_SIDES_COUNT));
        if (header.HasComment())
        {
          const auto& comment = stream.Get<RawComment>();
          if (const std::size_t size = comment.Size)
          {
            stream.GetData(size);
          }
        }
        const bool compressedData = id == ID_NEW;
        const bool newCompression = header.Version > 20;
        if (compressedData)
        {
          if (!newCompression)
          {
            Dbg("Old compression is not supported.");
            return 0;
          }
          const std::size_t packedSize = rawData.Size() - sizeof(header);
          const auto packed = rawData.GetSubcontainer(sizeof(header), packedSize);
          if (const auto fullDecoded =
                  Formats::Packed::Lha::DecodeRawDataAtLeast(*packed, COMPRESSION_ALGORITHM, MAX_IMAGE_SIZE))
          {
            SourceStream subStream(*fullDecoded);
            ParseSectors(subStream, visitor);
            const std::size_t usedInPacked = subStream.GetOffset();
            Dbg("Used {} bytes in packed stream", usedInPacked);
            if (const auto decoded =
                    Formats::Packed::Lha::DecodeRawDataAtLeast(*packed, COMPRESSION_ALGORITHM, usedInPacked))
            {
              const std::size_t usedSize = decoded->PackedSize();
              return sizeof(header) + usedSize;
            }
          }
          Dbg("Failed to decode lha stream");
          return 0;
        }
        else
        {
          ParseSectors(stream, visitor);
        }
        return stream.GetOffset();
      }
      catch (const std::exception&)
      {
        return 0;
      }
    }

    const Char DESCRIPTION[] = "TD0 (TeleDisk Image)";
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
        ""_sv;
  }  // namespace TeleDiskImage

  class TeleDiskImageDecoder : public Decoder
  {
  public:
    TeleDiskImageDecoder()
      : Format(Binary::CreateFormat(TeleDiskImage::FORMAT_PATTERN, TeleDiskImage::MIN_SIZE))
    {}

    String GetDescription() const override
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
        return Container::Ptr();
      }
      const Formats::ImageBuilder::Ptr builder = CreateSparsedImageBuilder();
      TeleDiskImage::ImageVisitorAdapter visitor(builder);
      if (const std::size_t usedSize = TeleDiskImage::Parse(rawData, visitor))
      {
        return CreateContainer(builder->GetResult(), usedSize);
      }
      return Container::Ptr();
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateTeleDiskImageDecoder()
  {
    return MakePtr<TeleDiskImageDecoder>();
  }
}  // namespace Formats::Packed
