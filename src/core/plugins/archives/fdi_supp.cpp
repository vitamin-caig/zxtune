/*
Abstract:
  FDI convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "core/plugins/registrator.h"
#include "core/src/callback.h"
#include "core/src/core.h"
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <cstring>
#include <numeric>
//boost includes
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 42F35DB2

namespace FullDiskImage
{
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    uint8_t ID[3];
    uint8_t ReadOnly;
    uint16_t Cylinders;
    uint16_t Sides;
    uint16_t TextOffset;
    uint16_t DataOffset;
    uint16_t InfoSize;
  } PACK_POST;

  PACK_PRE struct RawTrack
  {
    uint32_t Offset;
    uint16_t Reserved;
    uint8_t SectorsCount;
    PACK_PRE struct Sector
    {
      uint8_t Cylinder;
      uint8_t Head;
      uint8_t Number;
      uint8_t Size;
      uint8_t Flags;
      uint16_t Offset;
    } PACK_POST;
    Sector Sectors[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 14);
  BOOST_STATIC_ASSERT(sizeof(RawTrack::Sector) == 7);
  BOOST_STATIC_ASSERT(sizeof(RawTrack) == 14);

  const std::size_t FDI_MAX_SIZE = 1048576;
  const uint_t MIN_CYLINDERS_COUNT = 40;
  const uint_t MAX_CYLINDERS_COUNT = 100;
  const uint_t MIN_SIDES_COUNT = 1;
  const uint_t MAX_SIDES_COUNT = 2;
  const uint8_t FDI_ID[] = {'F', 'D', 'I'};

  struct SectorDescr
  {
    SectorDescr() : Num(), Begin(), End()
    {
    }
    SectorDescr(std::size_t num, const uint8_t* beg, const uint8_t* end) : Num(num), Begin(beg), End(end)
    {
    }
    uint_t Num;
    const uint8_t* Begin;
    const uint8_t* End;

    bool operator < (const SectorDescr& rh) const
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
    {
    }

    bool FastCheck() const
    {
      if (Size < sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      BOOST_STATIC_ASSERT(sizeof(header.ID) == sizeof(FDI_ID));
      if (0 != std::memcmp(header.ID, FDI_ID, sizeof(FDI_ID)))
      {
        return false;
      }
      const std::size_t dataOffset = fromLE(header.DataOffset);
      if (dataOffset < sizeof(header) ||
          dataOffset > Size)
      {
        return false;
      }
      const uint_t cylinders = fromLE(header.Cylinders);
      if (!in_range(cylinders, MIN_CYLINDERS_COUNT, MAX_CYLINDERS_COUNT))
      {
        return false;
      }
      const uint_t sides = fromLE(header.Sides);
      if (!in_range(sides, MIN_SIDES_COUNT, MAX_SIDES_COUNT))
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
      , UsedSize(0)
    {
    }

    const Dump* GetDecodedData()
    {
      if (IsValid && Decoded.empty())
      {
        IsValid = DecodeData();
      }
      return IsValid ? &Decoded : 0;
    }

    std::size_t GetUsedSize()
    {
      assert(IsValid && !Decoded.empty());
      return UsedSize;
    }
  private:
    bool DecodeData()
    {
      const uint8_t* const rawData = static_cast<const uint8_t*>(Header.ID);
      const std::size_t dataOffset = fromLE(Header.DataOffset);
      const uint_t cylinders = fromLE(Header.Cylinders);
      const uint_t sides = fromLE(Header.Sides);

      Dump result;
      result.reserve(FDI_MAX_SIZE);
      std::size_t trackInfoOffset = sizeof(Header) + fromLE(Header.InfoSize);
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
          //collect sectors reference
          SectorDescrs sectors;
          sectors.reserve(trackInfo->SectorsCount);
          for (std::size_t secNum = 0; secNum != trackInfo->SectorsCount; ++secNum)
          {
            const RawTrack::Sector* const sector = trackInfo->Sectors + secNum;
            const std::size_t secSize = 128 << sector->Size;
            //since there's no information about head number (always 0), do not check it
            //assert(sector->Head == sid);
            if (sector->Cylinder != cyl)
            {
              return false;
            }
            const std::size_t offset = dataOffset + fromLE(sector->Offset) + fromLE(trackInfo->Offset);
            if (offset + secSize > Limit)
            {
              return false;
            }
            sectors.push_back(SectorDescr(sector->Number, rawData + offset, rawData + offset + secSize));
            rawSize = std::max(rawSize, offset + secSize);
          }

          //sort by number
          std::sort(sectors.begin(), sectors.end());
          //and gather data
          for (SectorDescrs::const_iterator it = sectors.begin(), lim = sectors.end(); it != lim; ++it)
          {
            result.insert(result.end(), it->Begin, it->End);
          }
          //calculate next track by offset
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
    Dump Decoded;
    std::size_t UsedSize;
  };
}

namespace
{
  using namespace ZXTune;

  const Char FDI_PLUGIN_ID[] = {'F', 'D', 'I', 0};
  const String FDI_PLUGIN_VERSION(FromStdString("$Rev$"));

  IO::DataContainer::Ptr ExtractFDI(const IO::DataContainer& data, std::size_t& packedSize)
  {
    const FullDiskImage::Container container(data.Data(), data.Size());
    if (!container.FastCheck())
    {
      return IO::DataContainer::Ptr();
    }
    FullDiskImage::Decoder decoder(container);
    if (const Dump* res = decoder.GetDecodedData())
    {
      packedSize = decoder.GetUsedSize();
      return IO::CreateDataContainer(*res);
    }
    return IO::DataContainer::Ptr();
  }

  class FDIExtractionResult : public ArchiveExtractionResult
  {
  public:
    explicit FDIExtractionResult(IO::DataContainer::Ptr data)
      : RawData(data)
      , PackedSize(0)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      TryToExtract();
      return PackedSize;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      const uint_t size = RawData->Size();
      if (size < sizeof(FullDiskImage::RawHeader))
      {
        return size;
      }
      const uint8_t* const begin = static_cast<const uint8_t*>(RawData->Data());
      const uint8_t* const end = begin + size;
      return std::search(begin, end, FullDiskImage::FDI_ID, ArrayEnd(FullDiskImage::FDI_ID)) - begin;
    }

    virtual IO::DataContainer::Ptr GetExtractedData() const
    {
      TryToExtract();
      return ExtractedData;
    }
  private:
    void TryToExtract() const
    {
      if (PackedSize)
      {
        return;
      }
      ExtractedData = ExtractFDI(*RawData, PackedSize);
    }
  private:
    const IO::DataContainer::Ptr RawData;
    mutable std::size_t PackedSize;
    mutable IO::DataContainer::Ptr ExtractedData;
  };

  class FDIPlugin : public ArchivePlugin
                  , public boost::enable_shared_from_this<FDIPlugin>
  {
  public:
    virtual String Id() const
    {
      return FDI_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::FDI_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return FDI_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const FullDiskImage::Container container(inputData.Data(), inputData.Size());
      return container.FastCheck();
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr rawData = inputData->GetData();
      std::size_t packedSize = 0;
      if (const IO::DataContainer::Ptr subData = ExtractFDI(*rawData, packedSize))
      {
        const Plugin::Ptr plugin = shared_from_this();
        const ZXTune::Module::NoProgressDetectCallbackAdapter noProgressCallback(callback);
        const String subPath = Text::ARCHIVE_PLUGIN_PREFIX + plugin->Id();
        const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(inputData, plugin, subData, subPath);
        ZXTune::Module::Detect(subLocation, noProgressCallback);
        return DetectionResult::Create(packedSize, 0);
      }
      return boost::make_shared<FDIExtractionResult>(rawData);
    }

    virtual ArchiveExtractionResult::Ptr ExtractSubdata(IO::DataContainer::Ptr input) const
    {
      return boost::make_shared<FDIExtractionResult>(input);
    }
  };
}

namespace ZXTune
{
  void RegisterFDIConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new FDIPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
