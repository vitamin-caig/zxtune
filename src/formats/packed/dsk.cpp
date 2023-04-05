/**
 *
 * @file
 *
 * @brief  DSK images support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
#include "formats/packed/image_utils.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/packed.h>
// std includes
#include <array>

namespace Formats::Packed
{
  namespace DSK
  {
    typedef std::array<uint8_t, 34> DiskSignatureType;

    const DiskSignatureType DISK_SIGNATURE = {{'M', 'V', ' ', '-', ' ', 'C', 'P', 'C', 'E',  'M',  'U',  ' ',
                                               'D', 'i', 's', 'k', '-', 'F', 'i', 'l', 'e',  '\r', '\n', 'D',
                                               'i', 's', 'k', '-', 'I', 'n', 'f', 'o', '\r', '\n'}};

    struct DiskInformationBlock
    {
      DiskSignatureType Signature;
      uint8_t Creator[14];
      uint8_t Tracks;
      uint8_t Sides;
      le_uint16_t TrackSize;
      uint8_t Unused[204];

      uint_t GetTrackSize() const
      {
        return TrackSize;
      }
    };

    typedef std::array<uint8_t, 12> TrackSignatureType;

    const TrackSignatureType TRACK_SIGNATURE = {{'T', 'r', 'a', 'c', 'k', '-', 'I', 'n', 'f', 'o', '\r', '\n'}};

    struct TrackInformationBlock
    {
      struct SectorInfo
      {
        uint8_t Track;
        uint8_t Side;
        uint8_t Sector;
        uint8_t Size;
        uint8_t FDCStatus[2];
        le_uint16_t ActualDataSize;
      };

      TrackSignatureType Signature;
      uint8_t Unused[4];
      uint8_t Track;
      uint8_t Side;
      uint8_t Unused2[2];
      uint8_t SectorSize;
      uint8_t SectorsCount;
      uint8_t GAPLength;
      uint8_t Filler;
      std::array<SectorInfo, 29> Sectors;
    };

    const DiskSignatureType EXTENDED_DISK_SIGNATURE = {{'E', 'X', 'T', 'E', 'N', 'D', 'E', 'D', ' ',  'C',  'P',  'C',
                                                        ' ', 'D', 'S', 'K', ' ', 'F', 'i', 'l', 'e',  '\r', '\n', 'D',
                                                        'i', 's', 'k', '-', 'I', 'n', 'f', 'o', '\r', '\n'}};

    struct ExtendedDiskInformationBlock
    {
      DiskSignatureType Signature;
      uint8_t Creator[14];
      uint8_t Tracks;
      uint8_t Sides;
      uint8_t Unused[2];
      uint8_t TrackSizes[204];

      uint_t GetTrackSize(uint_t trackIdx) const
      {
        return 256 * TrackSizes[trackIdx];
      }
    };

    static_assert(sizeof(DiskInformationBlock) * alignof(DiskInformationBlock) == 256, "Invalid layout");
    static_assert(sizeof(TrackInformationBlock::SectorInfo) * alignof(TrackInformationBlock::SectorInfo) == 8,
                  "Invalid layout");
    static_assert(sizeof(TrackInformationBlock) * alignof(TrackInformationBlock) == 256, "Invalid layout");
    static_assert(sizeof(ExtendedDiskInformationBlock) * alignof(ExtendedDiskInformationBlock) == 256,
                  "Invalid layout");

    inline std::size_t GetSectorDataSize(uint_t sectorSize)
    {
      // For 8k Sectors (N="6"), only 1800h bytes is stored
      return sectorSize == 6 ? 0x1800 : (128 << (sectorSize & 3));
    }

    class Format
    {
    public:
      explicit Format(Formats::ImageBuilder& target)
        : Target(target)
      {}

      std::size_t Parse(Binary::View diskData)
      {
        Require(diskData.Size() > sizeof(DiskSignatureType));
        const DiskSignatureType& signature = *safe_ptr_cast<const DiskSignatureType*>(diskData.Start());
        if (signature == DISK_SIGNATURE)
        {
          return ParseBase(diskData);
        }
        else if (signature == EXTENDED_DISK_SIGNATURE)
        {
          return ParseExtended(diskData);
        }
        else
        {
          throw std::exception();
        }
      }

      std::size_t ParseBase(Binary::View diskData)
      {
        Binary::DataInputStream diskStream(diskData);
        const auto& diskInfo = diskStream.Read<DiskInformationBlock>();
        Require(diskInfo.Signature == DISK_SIGNATURE);
        for (uint_t track = 0; track != diskInfo.Tracks; ++track)
        {
          for (uint_t side = 0; side != diskInfo.Sides; ++side)
          {
            const std::size_t trackSize = diskInfo.GetTrackSize();
            const auto trackData = diskStream.ReadData(trackSize);
            ParseTrack(trackData);
          }
        }
        return diskStream.GetPosition();
      }

      std::size_t ParseExtended(Binary::View diskData)
      {
        Binary::DataInputStream diskStream(diskData);
        const auto& diskInfo = diskStream.Read<ExtendedDiskInformationBlock>();
        Require(diskInfo.Signature == EXTENDED_DISK_SIGNATURE);
        for (uint_t track = 0, cylinder = 0; track != diskInfo.Tracks; ++track)
        {
          for (uint_t side = 0; side != diskInfo.Sides; ++side, ++cylinder)
          {
            if (const std::size_t trackSize = diskInfo.GetTrackSize(cylinder))
            {
              const auto trackData = diskStream.ReadData(trackSize);
              ParseExtendedTrack(trackData);
            }
          }
        }
        return diskStream.GetPosition();
      }

    private:
      void ParseTrack(Binary::View trackData)
      {
        Binary::DataInputStream trackStream(trackData);
        const auto& trackInfo = trackStream.Read<TrackInformationBlock>();
        Require(trackInfo.Signature == TRACK_SIGNATURE);
        Require(trackInfo.SectorsCount <= trackInfo.Sectors.size());
        for (uint_t sector = 0; sector != trackInfo.SectorsCount; ++sector)
        {
          const TrackInformationBlock::SectorInfo& sectorInfo = trackInfo.Sectors[sector];
          const std::size_t rawSectorSize = GetSectorDataSize(trackInfo.SectorSize);
          const std::size_t usedSectorSize = GetSectorDataSize(sectorInfo.Size);
          Require(rawSectorSize >= usedSectorSize);
          const auto sectorData = trackStream.ReadData(rawSectorSize);
          Target.SetSector(Formats::CHS(sectorInfo.Track, sectorInfo.Side, sectorInfo.Sector), sectorData);
        }
      }

      void ParseExtendedTrack(Binary::View trackData)
      {
        Binary::DataInputStream trackStream(trackData);
        const auto& trackInfo = trackStream.Read<TrackInformationBlock>();
        Require(trackInfo.Signature == TRACK_SIGNATURE);
        Require(trackInfo.SectorsCount <= trackInfo.Sectors.size());
        for (uint_t sector = 0; sector != trackInfo.SectorsCount; ++sector)
        {
          const TrackInformationBlock::SectorInfo& sectorInfo = trackInfo.Sectors[sector];
          if (const std::size_t sectorSize = sectorInfo.ActualDataSize)
          {
            const auto sectorData = trackStream.ReadData(sectorSize).As<uint8_t>();
            Target.SetSector(Formats::CHS(sectorInfo.Track, sectorInfo.Side, sectorInfo.Sector),
                             Binary::View(sectorData, sectorSize));
          }
        }
      }

    private:
      Formats::ImageBuilder& Target;
    };

    const Char DESCRIPTION[] = "DSK Image";
    const auto FORMAT =
        "'M|'E"
        "'V|'X"
        "' |'T"
        "'-|'E"
        "' |'N"
        "'C|'D"
        "'P|'E"
        "'C|'D"
        "'E|' "
        "'M|'C"
        "'U|'P"
        "' |'C"
        "'D|' "
        "'i|'D"
        "'s|'S"
        "'k|'K"
        "'-|' "
        "'F'i'l'e'\r'\n'D'i's'k'-'I'n'f'o'\r'\n"  // signature
        "?{14}"                                   // creator
        "01-64"                                   // tracks
        "01-02"                                   // sides
        "?{206}"                                  // skipped
        // first track
        "'T'r'a'c'k'-'I'n'f'o'\r'\n"
        ""_sv;
  }  // namespace DSK

  class DSKDecoder : public Decoder
  {
  public:
    DSKDecoder()
      : Format(Binary::CreateFormat(DSK::FORMAT))
    {}

    String GetDescription() const override
    {
      return DSK::DESCRIPTION;
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
      try
      {
        const Formats::ImageBuilder::Ptr builder = CreateSparsedImageBuilder();
        const std::size_t usedSize = DSK::Format(*builder).Parse(rawData);
        return CreateContainer(builder->GetResult(), usedSize);
      }
      catch (const std::exception&)
      {
        return Container::Ptr();
      }
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateDSKDecoder()
  {
    return MakePtr<DSKDecoder>();
  }
}  // namespace Formats::Packed
