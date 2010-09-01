/*
Abstract:
  FDI implicit convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/plugins/enumerator.h>
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <numeric>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 42F35DB2

namespace
{
  using namespace ZXTune;

  const Char FDI_PLUGIN_ID[] = {'F', 'D', 'I', 0};
  const String FDI_PLUGIN_VERSION(FromStdString("$Rev$"));

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct FDIHeader
  {
    uint8_t ID[3];
    uint8_t ReadOnly;
    uint16_t Cylinders;
    uint16_t Sides;
    uint16_t TextOffset;
    uint16_t DataOffset;
    uint16_t InfoSize;
  } PACK_POST;

  PACK_PRE struct FDITrack
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

  BOOST_STATIC_ASSERT(sizeof(FDIHeader) == 14);
  BOOST_STATIC_ASSERT(sizeof(FDITrack::Sector) == 7);
  BOOST_STATIC_ASSERT(sizeof(FDITrack) == 14);

  const std::size_t FDI_MAX_SIZE = 1048576;
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

  //////////////////////////////////////////////////////////////////////////
  class FDIPlugin : public ImplicitPlugin
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
      const std::size_t limit = inputData.Size();
      if (limit < sizeof(FDIHeader))
      {
        return false;
      }
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const FDIHeader* const header = safe_ptr_cast<const FDIHeader*>(data);
      BOOST_STATIC_ASSERT(sizeof(header->ID) == sizeof(FDI_ID));
      if (0 != std::memcmp(header->ID, FDI_ID, sizeof(FDI_ID)))
      {
        return false;
      }
      const std::size_t dataOffset = fromLE(header->DataOffset);
      const uint_t cylinders = fromLE(header->Cylinders);
      const uint_t sides = fromLE(header->Sides);
      if (dataOffset < sizeof(*header) || dataOffset > limit ||
          !cylinders || !sides)
      {
        return false;
      }

      const FDITrack* trackInfo = safe_ptr_cast<const FDITrack*>(data + sizeof(*header) + fromLE(header->InfoSize));
      std::size_t rawSize = dataOffset;
      for (uint_t cyl = 0; cyl != cylinders; ++cyl)
      {
        for (uint_t sid = 0; sid != sides; ++sid)
        {
          for (std::size_t secNum = 0; secNum != trackInfo->SectorsCount; ++secNum)
          {
            const FDITrack::Sector* const sector = trackInfo->Sectors + secNum;
            const std::size_t secSize = 128 << sector->Size;
            //since there's no information about head number (always 0), do not check it
            //assert(sector->Head == sid);
            if (sector->Cylinder != cyl)
            {
              return false;
            }
            const std::size_t offset = dataOffset + fromLE(sector->Offset) + fromLE(trackInfo->Offset);
            if (offset + secSize > limit)
            {
              return false;
            }
          }
        }
      }
      return true;
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Map& /*commonParams*/,
      const MetaContainer& input, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *input.Data;
      const std::size_t limit = inputData.Size();
      if (!Check(inputData))
      {
        return IO::DataContainer::Ptr();
      }

      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const FDIHeader* const header = safe_ptr_cast<const FDIHeader*>(data);
      const std::size_t dataOffset = fromLE(header->DataOffset);
      const uint_t cylinders = fromLE(header->Cylinders);
      const uint_t sides = fromLE(header->Sides);

      Dump buffer;
      buffer.reserve(FDI_MAX_SIZE);
      const FDITrack* trackInfo = safe_ptr_cast<const FDITrack*>(data + sizeof(*header) + fromLE(header->InfoSize));
      std::size_t rawSize = dataOffset;
      for (uint_t cyl = 0; cyl != cylinders; ++cyl)
      {
        for (uint_t sid = 0; sid != sides; ++sid)
        {
          typedef std::vector<SectorDescr> SectorDescrs;
          //collect sectors reference
          SectorDescrs sectors;
          sectors.reserve(trackInfo->SectorsCount);
          for (std::size_t secNum = 0; secNum != trackInfo->SectorsCount; ++secNum)
          {
            const FDITrack::Sector* const sector = trackInfo->Sectors + secNum;
            const std::size_t secSize = 128 << sector->Size;
            const std::size_t offset = dataOffset + fromLE(sector->Offset) + fromLE(trackInfo->Offset);
            sectors.push_back(SectorDescr(sector->Number, data + offset, data + offset + secSize));
            rawSize = std::max(rawSize, offset + secSize);
          }

          //sort by number
          std::sort(sectors.begin(), sectors.end());
          //and gather data
          for (SectorDescrs::const_iterator it = sectors.begin(), lim = sectors.end(); it != lim; ++it)
          {
            buffer.insert(buffer.end(), it->Begin, it->End);
          }
          //calculate next track by offset
          trackInfo = safe_ptr_cast<const FDITrack*>(safe_ptr_cast<const uint8_t*>(trackInfo) +
            sizeof(*trackInfo) + (trackInfo->SectorsCount - 1) * sizeof(trackInfo->Sectors));
        }
      }
      region.Offset = 0;
      region.Size = rawSize;
      return IO::CreateDataContainer(buffer);
    }
  };
}

namespace ZXTune
{
  void RegisterFDIConvertor(PluginsEnumerator& enumerator)
  {
    const ImplicitPlugin::Ptr plugin(new FDIPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
