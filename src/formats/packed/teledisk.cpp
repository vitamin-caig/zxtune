/**
* 
* @file
*
* @brief  TeleDisk images support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container.h"
#include "image_utils.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <contract.h>
//library includes
#include <binary/typed_container.h>
#include <debug/log.h>
#include <formats/packed.h>
#include <formats/packed/lha_supp.h>
#include <math/numeric.h>
//std includes
#include <cstring>
#include <numeric>
//text includes
#include <formats/text/packed.h>

namespace
{
  const Debug::Stream Dbg("Formats::Packed::Teledisk");
}

namespace TeleDiskImage
{
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    uint16_t ID;
    uint8_t Sequence;
    uint8_t CheckSequence;
    uint8_t Version;
    uint8_t DataRate;
    uint8_t DriveType;
    uint8_t Stepping;
    uint8_t DOSAllocation;
    uint8_t Sides;
    uint16_t CRC;

    bool HasComment() const
    {
      return 0 != (Stepping & 0x80);
    }
  } PACK_POST;

  PACK_PRE struct RawComment
  {
    uint16_t CRC;
    uint16_t Size;
    uint8_t Year;
    uint8_t Month;
    uint8_t Day;
    uint8_t Hour;
    uint8_t Minute;
    uint8_t Second;
  } PACK_POST;

  PACK_PRE struct RawTrack
  {
    uint8_t Sectors;
    uint8_t Cylinder;
    uint8_t Head;
    uint8_t CRC;

    bool IsLast() const
    {
      return Sectors == 0xff;
    }
  } PACK_POST;

  PACK_PRE struct RawSector
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
  } PACK_POST;

  PACK_PRE struct RawData
  {
    uint16_t Size;
    uint8_t Method;
  } PACK_POST;

  PACK_PRE struct R2PEntry
  {
    uint16_t Count;
    uint8_t Data[2];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 12);
  BOOST_STATIC_ASSERT(sizeof(RawComment) == 10);
  BOOST_STATIC_ASSERT(sizeof(RawTrack) == 4);
  BOOST_STATIC_ASSERT(sizeof(RawSector) == 6);
  BOOST_STATIC_ASSERT(sizeof(RawData) == 3);
  BOOST_STATIC_ASSERT(sizeof(R2PEntry) == 4);

  const uint_t MAX_CYLINDERS_COUNT = 100;
  const uint_t MIN_SIDES_COUNT = 1;
  const uint_t MAX_SIDES_COUNT = 2;
  const std::size_t MAX_SECTOR_SIZE = 8192;

  const uint_t ID_OLD = 0x4454;
  const uint_t ID_NEW = 0x6474;

  const std::size_t MIN_SIZE = sizeof(RawHeader);
  const std::size_t MAX_IMAGE_SIZE = 1048576;
  const std::string COMPRESSION_ALGORITHM("-lh1-");

  enum SectorDataType
  {
    RAW_SECTOR = 0,
    R2P_SECTOR,
    RLE_SECTOR
  };

  class ImageVisitor
  {
  public:
    virtual ~ImageVisitor() {}

    virtual void OnSector(const Formats::CHS& loc, const uint8_t* rawData, std::size_t rawSize, SectorDataType type, std::size_t targetSize) = 0;
  };

  class StubImageVisitor : public ImageVisitor
  {
  public:
    virtual void OnSector(const Formats::CHS& /*loc*/, const uint8_t* /*rawData*/, std::size_t rawSize, SectorDataType type, std::size_t targetSize)
    {
      switch (type)
      {
      case RAW_SECTOR:
        Require(rawSize == targetSize);
        break;
      case R2P_SECTOR:
        Require(rawSize % sizeof(R2PEntry) == 0);
        break;
      default:
        break;
      }
    }
  };

  void DecodeR2P(const uint8_t* data, std::size_t size, Dump& result)
  {
    Require(size % sizeof(R2PEntry) == 0);
    Dump tmp;
    tmp.reserve(MAX_SECTOR_SIZE);
    for (const R2PEntry* it = safe_ptr_cast<const R2PEntry*>(data), *lim = it + size / sizeof(*it); it != lim; ++it)
    {
      const uint_t count = fromLE(it->Count);
      Require(count != 0);
      tmp.push_back(it->Data[0]);
      tmp.push_back(it->Data[1]);
      Require(CopyFromBack(sizeof(it->Data), tmp, sizeof(it->Data) * (count - 1)));
    }
    result.swap(tmp);
  }

  void DecodeRLE(const uint8_t* data, std::size_t size, Dump& result)
  {
    Dump tmp;
    tmp.reserve(MAX_SECTOR_SIZE);
    ByteStream stream(data, size);
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
        tmp.push_back(stream.GetByte());
      }
      if (isRLE)
      {
        Require(CopyFromBack(len, tmp, len * (count - 1)));
      }
    }
    result.swap(tmp);
  }

  class ImageVisitorAdapter : public ImageVisitor
  {
  public:
    explicit ImageVisitorAdapter(Formats::ImageBuilder::Ptr builder)
      : Builder(builder)
    {
    }

    virtual void OnSector(const Formats::CHS& loc, const uint8_t* rawData, std::size_t rawSize, SectorDataType type, std::size_t targetSize)
    {
      Dump result;
      switch (type)
      {
      case RAW_SECTOR:
        result.assign(rawData, rawData + rawSize);
        break;
      case R2P_SECTOR:
        DecodeR2P(rawData, rawSize, result);
        break;
      case RLE_SECTOR:
        DecodeRLE(rawData, rawSize, result);
        break;
      }
      Require(result.size() == targetSize);
      Builder->SetSector(loc, result);
    }
  private:
    const Formats::ImageBuilder::Ptr Builder;
  };

  class SourceStream
  {
  public:
    explicit SourceStream(const Binary::Container& rawData)
      : Data(rawData)
      , Offset(0)
    {
    }

    template<class T>
    const T& Get()
    {
      const T* const res = Data.GetField<T>(Offset);
      Require(res != 0);
      Offset += sizeof(*res);
      return *res;
    }

    const uint8_t* GetData(std::size_t size)
    {
      Require(size != 0);
      const uint8_t* const first = Data.GetField<uint8_t>(Offset);
      const uint8_t* const last = Data.GetField<uint8_t>(Offset + size - 1);
      Require(first != 0 && last != 0);
      Offset += size;
      return first;
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }
  private:
    const Binary::TypedContainer Data;
    std::size_t Offset;
  };

  void ParseSectors(SourceStream& stream, ImageVisitor& visitor)
  {
    for (;;)
    {
      const RawTrack& track = stream.Get<RawTrack>();
      if (track.IsLast())
      {
        break;
      }
      Require(Math::InRange<uint_t>(track.Cylinder, 0, MAX_CYLINDERS_COUNT));
      for (uint_t sect = 0; sect != track.Sectors; ++sect)
      {
        const RawSector& sector = stream.Get<RawSector>();
        if (sector.NoData())
        {
          continue;
        }
        Require(Math::InRange<uint_t>(sector.Size, 0, 6));
        const std::size_t sectorSize = 128 << sector.Size;
        const RawData& srcDataDesc = stream.Get<RawData>();
        Require(Math::InRange<uint_t>(srcDataDesc.Method, RAW_SECTOR, RLE_SECTOR));
        const std::size_t dataSize = fromLE(srcDataDesc.Size) - 1;
        const uint8_t* const rawData = stream.GetData(dataSize);
        //use track parameters for layout
        if (!sector.NoId())
        {
          const Formats::CHS loc(sector.Cylinder, track.Head, sector.Number);
          visitor.OnSector(loc, rawData, dataSize, static_cast<SectorDataType>(srcDataDesc.Method), sectorSize);
        }
      }
    }
  }

  std::size_t Parse(const Binary::Container& rawData, ImageVisitor& visitor)
  {
    SourceStream stream(rawData);
    try
    {
      const RawHeader& header = stream.Get<RawHeader>();
      const uint_t id = fromLE(header.ID);
      Require(id == ID_OLD || id == ID_NEW);
      Require(header.Sequence == 0);
      Require(Math::InRange<uint_t>(header.Sides, MIN_SIDES_COUNT, MAX_SIDES_COUNT));
      if (header.HasComment())
      {
        const RawComment& comment = stream.Get<RawComment>();
        if (const std::size_t size = fromLE(comment.Size))
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
        const Binary::Container::Ptr packed = rawData.GetSubcontainer(sizeof(header), packedSize);
        if (const Formats::Packed::Container::Ptr fullDecoded = 
          Formats::Packed::Lha::DecodeRawDataAtLeast(*packed, COMPRESSION_ALGORITHM, MAX_IMAGE_SIZE))
        {
          SourceStream subStream(*fullDecoded);
          ParseSectors(subStream, visitor);
          const std::size_t usedInPacked = subStream.GetOffset();
          Dbg("Used %1% bytes in packed stream", usedInPacked);
          if (const Formats::Packed::Container::Ptr decoded =
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

  const std::string FORMAT_PATTERN(
    "('T|'t)('D|'d)"        // uint8_t ID[2]
    "00"          // uint8_t Sequence;
    "?"           // uint8_t CheckSequence;
    "?"           // uint8_t Version;
    "%x00000xx"   // uint8_t DataRate;
    "00-06"       // uint8_t DriveType;
    "%x00000xx"   // uint8_t Stepping;
    /*
    "?"           // uint8_t DOSAllocation;
    "?"           // uint8_t Sides;
    "??"          // uint16_t CRC;
    */
  );
}

namespace Formats
{
  namespace Packed
  {
    class TeleDiskImageDecoder : public Decoder
    {
    public:
      TeleDiskImageDecoder()
        : Format(Binary::Format::Create(TeleDiskImage::FORMAT_PATTERN, TeleDiskImage::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::TELEDISKIMAGE_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (!Format->Match(rawData))
        {
          return Container::Ptr();
        }
        const Formats::ImageBuilder::Ptr builder = CreateSparsedImageBuilder();
        TeleDiskImage::ImageVisitorAdapter visitor(builder);
        if (const std::size_t usedSize = TeleDiskImage::Parse(rawData, visitor))
        {
          return CreatePackedContainer(builder->GetResult(), usedSize);
        }
        return Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateTeleDiskImageDecoder()
    {
      return boost::make_shared<TeleDiskImageDecoder>();
    }
  }
}
