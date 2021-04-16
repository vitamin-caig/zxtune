/**
 *
 * @file
 *
 * @brief  FullDiskImage images support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
#include <math/numeric.h>
// std includes
#include <cassert>
#include <cstring>
#include <numeric>

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

    struct SectorDescr
    {
      SectorDescr()
        : Num()
        , Begin()
        , End()
      {}
      SectorDescr(uint_t num, const uint8_t* beg, const uint8_t* end)
        : Num(num)
        , Begin(beg)
        , End(end)
      {}
      uint_t Num;
      const uint8_t* Begin;
      const uint8_t* End;

      bool operator<(const SectorDescr& rh) const
      {
        return Num < rh.Num;
      }
    };

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
        if (!Math::InRange(sides, MIN_SIDES_COUNT, MAX_SIDES_COUNT))
        {
          return false;
        }
        return true;
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
        , Result(new Binary::Dump())
        , Decoded(*Result)
        , UsedSize(0)
      {
        IsValid = DecodeData();
      }

      std::unique_ptr<Binary::Dump> GetResult()
      {
        return IsValid ? std::move(Result) : std::unique_ptr<Binary::Dump>();
      }

      std::size_t GetUsedSize()
      {
        return UsedSize;
      }

    private:
      bool DecodeData()
      {
        const uint8_t* const rawData = static_cast<const uint8_t*>(Header.ID);
        const std::size_t dataOffset = Header.DataOffset;
        const uint_t cylinders = Header.Cylinders;
        const uint_t sides = Header.Sides;

        Binary::Dump result;
        result.reserve(FDI_MAX_SIZE);
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

            const RawTrack* const trackInfo = safe_ptr_cast<const RawTrack*>(rawData + trackInfoOffset);
            typedef std::vector<SectorDescr> SectorDescrs;
            // collect sectors reference
            SectorDescrs sectors;
            sectors.reserve(trackInfo->SectorsCount);
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
              sectors.push_back(SectorDescr(sector->Number, rawData + offset, rawData + offset + secSize));
              rawSize = std::max(rawSize, offset + secSize);
            }

            // sort by number
            std::sort(sectors.begin(), sectors.end());
            // and gather data
            for (SectorDescrs::const_iterator it = sectors.begin(), lim = sectors.end(); it != lim; ++it)
            {
              result.insert(result.end(), it->Begin, it->End);
            }
            // calculate next track by offset
            trackInfoOffset += sizeof(*trackInfo) + (trackInfo->SectorsCount - 1) * sizeof(trackInfo->Sectors);
          }
        }
        UsedSize = rawSize;
        Decoded.swap(result);
        return true;
      }

    private:
      bool IsValid;
      const RawHeader& Header;
      const std::size_t Limit;
      std::unique_ptr<Binary::Dump> Result;
      Binary::Dump& Decoded;
      std::size_t UsedSize;
    };

    const Char DESCRIPTION[] = "FDI (Full Disk Image)";
    const auto FORMAT_PATTERN =
        "'F'D'I"     // uint8_t ID[3]
        "%0000000x"  // uint8_t ReadOnly;
        "28-64 00"   // uint16_t Cylinders;
        "01-02 00"   // uint16_t Sides;
        ""_sv;
  }  // namespace FullDiskImage

  class FullDiskImageDecoder : public Decoder
  {
  public:
    FullDiskImageDecoder()
      : Format(Binary::CreateFormat(FullDiskImage::FORMAT_PATTERN))
    {}

    String GetDescription() const override
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
        return Container::Ptr();
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
