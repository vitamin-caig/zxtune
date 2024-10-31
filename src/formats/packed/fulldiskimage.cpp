/**
 *
 * @file
 *
 * @brief  FullDiskImage images support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/packed/container.h"

#include "binary/data_builder.h"
#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/view.h"
#include "formats/packed.h"
#include "math/numeric.h"

#include "byteorder.h"
#include "make_ptr.h"
#include "pointers.h"
#include "string_view.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <utility>

namespace Formats::Packed
{
  namespace FullDiskImage
  {
    struct RawHeader
    {
      uint8_t ID[3];
      uint8_t ReadOnly;
      le_uint16_t Cylinders;
      le_uint16_t Sides;
      le_uint16_t TextOffset;
      le_uint16_t DataOffset;
      le_uint16_t InfoSize;
    };

    struct RawTrack
    {
      le_uint32_t Offset;
      le_uint16_t Reserved;
      uint8_t SectorsCount;
      struct Sector
      {
        uint8_t Cylinder;
        uint8_t Head;
        uint8_t Number;
        uint8_t Size;
        uint8_t Flags;
        le_uint16_t Offset;
      };
      Sector Sectors[1];
    };

    static_assert(sizeof(RawHeader) * alignof(RawHeader) == 14, "Invalid layout");
    static_assert(sizeof(RawTrack::Sector) * alignof(RawTrack::Sector) == 7, "Invalid layout");
    static_assert(sizeof(RawTrack) * alignof(RawTrack) == 14, "Invalid layout");

    const std::size_t FDI_MAX_SIZE = 1048576;
    const uint_t MIN_CYLINDERS_COUNT = 40;
    const uint_t MAX_CYLINDERS_COUNT = 100;
    const uint_t MIN_SIDES_COUNT = 1;
    const uint_t MAX_SIDES_COUNT = 2;
    const uint8_t FDI_ID[] = {'F', 'D', 'I'};

    class Container
    {
    public:
      Container(const void* data, std::size_t size)
        : Data(static_cast<const uint8_t*>(data))
        , Size(size)
      {}

      bool FastCheck() const
      {
        if (Size < sizeof(RawHeader))
        {
          return false;
        }
        const RawHeader& header = GetHeader();
        static_assert(sizeof(header.ID) == sizeof(FDI_ID), "Invalid layout");
        if (0 != std::memcmp(header.ID, FDI_ID, sizeof(FDI_ID)))
        {
          return false;
        }
        const std::size_t dataOffset = header.DataOffset;
        if (dataOffset < sizeof(header) || dataOffset > Size)
        {
          return false;
        }
        const uint_t cylinders = header.Cylinders;
        if (!Math::InRange(cylinders, MIN_CYLINDERS_COUNT, MAX_CYLINDERS_COUNT))
        {
          return false;
        }
        const uint_t sides = header.Sides;
        return Math::InRange(sides, MIN_SIDES_COUNT, MAX_SIDES_COUNT);
      }

      const RawHeader& GetHeader() const
      {
        assert(Size >= sizeof(RawHeader));
        return *safe_ptr_cast<const RawHeader*>(Data);
      }

      std::size_t GetSize() const
      {
        return Size;
      }

    private:
      const uint8_t* const Data;
      const std::size_t Size;
    };

    class Decoder
    {
    public:
      explicit Decoder(const Container& container)
        : IsValid(container.FastCheck())
        , Header(container.GetHeader())
        , Limit(container.GetSize())
        , Result(FDI_MAX_SIZE)
      {
        IsValid = DecodeData();
      }

      Binary::Container::Ptr GetResult()
      {
        return IsValid ? Result.CaptureResult() : Binary::Container::Ptr();
      }

      std::size_t GetUsedSize() const
      {
        return UsedSize;
      }

    private:
      bool DecodeData()
      {
        const auto* const rawData = static_cast<const uint8_t*>(Header.ID);
        const std::size_t dataOffset = Header.DataOffset;
        const uint_t cylinders = Header.Cylinders;
        const uint_t sides = Header.Sides;

        std::size_t trackInfoOffset = sizeof(Header) + Header.InfoSize;
        std::size_t rawSize = dataOffset;
        for (uint_t cyl = 0; cyl != cylinders; ++cyl)
        {
          for (uint_t sid = 0; sid != sides; ++sid)
          {
            if (trackInfoOffset + sizeof(RawTrack) > Limit)
            {
              return false;
            }

            const auto* const trackInfo = safe_ptr_cast<const RawTrack*>(rawData + trackInfoOffset);
            // collect sectors reference
            std::map<uint_t, Binary::View> sectors;
            for (std::size_t secNum = 0; secNum != trackInfo->SectorsCount; ++secNum)
            {
              const RawTrack::Sector* const sector = trackInfo->Sectors + secNum;
              const std::size_t secSize = std::size_t(128) << sector->Size;
              // since there's no information about head number (always 0), do not check it
              // assert(sector->Head == sid);
              if (sector->Cylinder != cyl)
              {
                return false;
              }
              const std::size_t offset = dataOffset + sector->Offset + trackInfo->Offset;
              if (offset + secSize > Limit)
              {
                return false;
              }
              sectors.emplace(sector->Number, Binary::View{rawData + offset, secSize});
              rawSize = std::max(rawSize, offset + secSize);
            }

            // and gather data
            for (const auto& s : sectors)
            {
              Result.Add(s.second);
            }
            // calculate next track by offset
            trackInfoOffset += sizeof(*trackInfo) + (trackInfo->SectorsCount - 1) * sizeof(trackInfo->Sectors);
          }
        }
        UsedSize = rawSize;
        return true;
      }

    private:
      bool IsValid;
      const RawHeader& Header;
      const std::size_t Limit;
      Binary::DataBuilder Result;
      std::size_t UsedSize = 0;
    };

    const auto DESCRIPTION = "FDI (Full Disk Image)"sv;
    const auto FORMAT_PATTERN =
        "'F'D'I"     // uint8_t ID[3]
        "%0000000x"  // uint8_t ReadOnly;
        "28-64 00"   // uint16_t Cylinders;
        "01-02 00"   // uint16_t Sides;
        ""sv;
  }  // namespace FullDiskImage

  class FullDiskImageDecoder : public Decoder
  {
  public:
    FullDiskImageDecoder()
      : Format(Binary::CreateFormat(FullDiskImage::FORMAT_PATTERN))
    {}

    StringView GetDescription() const override
    {
      return FullDiskImage::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      const void* const data = rawData.Start();
      const std::size_t availSize = rawData.Size();
      const FullDiskImage::Container container(data, availSize);
      if (!container.FastCheck())
      {
        return {};
      }
      FullDiskImage::Decoder decoder(container);
      return CreateContainer(decoder.GetResult(), decoder.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateFullDiskImageDecoder()
  {
    return MakePtr<FullDiskImageDecoder>();
  }
}  // namespace Formats::Packed
