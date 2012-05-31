/*
Abstract:
  TRD containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "trdos_catalogue.h"
#include "trdos_utils.h"
//common includes
#include <byteorder.h>
#include <logging.h>
#include <range_checker.h>
#include <tools.h>
//std includes
#include <cstring>
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>
//text include
#include <formats/text/archived.h>

namespace TRD
{
  using namespace Formats;

  const std::string FORMAT(
    "(00|01|20-7f??????? ??? ?? ? 0x 00-a0){128}"
    //service sector
    "00"     //zero
    "+224+"  //reserved
    "?"      //free sector
    "?"      //free track
    "16"     //type DS_DD
    "01-7f"  //files
    "?00-09" //free sectors
    "10"     //ID
    "0000+9+00"//reserved
    "?"      //deleted files
    "20-7f{8}"//title
    "000000"  //reserved
  );

  const std::string THIS_MODULE("Formats::Archived::TRD");

  //hints
  const std::size_t MODULE_SIZE = 655360;
  const uint_t BYTES_PER_SECTOR = 256;
  const uint_t SECTORS_IN_TRACK = 16;
  const uint_t MAX_FILES_COUNT = 128;
  const uint_t SERVICE_SECTOR_NUM = 8;
  const std::size_t MIN_SIZE = (SECTORS_IN_TRACK + 1) * BYTES_PER_SECTOR;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif

  enum
  {
    NOENTRY = 0,
    DELETED = 1
  };
  PACK_PRE struct CatEntry
  {
    char Name[8];
    char Type[3];
    uint16_t Length;
    uint8_t SizeInSectors;
    uint8_t Sector;
    uint8_t Track;
  } PACK_POST;

  enum
  {
    TRDOS_ID = 0x10,

    DS_DD = 0x16,
    DS_SD = 0x17,
    SS_DD = 0x18,
    SS_SD = 0x19
  };

  PACK_PRE struct ServiceSector
  {
    uint8_t Zero;
    uint8_t Reserved1[224];
    uint8_t FreeSpaceSect;
    uint8_t FreeSpaceTrack;
    uint8_t Type;
    uint8_t Files;
    uint16_t FreeSectors;
    uint8_t ID;//0x10
    uint8_t Reserved2[12];
    uint8_t DeletedFiles;
    uint8_t Title[8];
    uint8_t Reserved3[3];
  } PACK_POST;

  struct Sector
  {
    uint8_t Content[BYTES_PER_SECTOR];

    bool IsEmpty() const
    {
      return ArrayEnd(Content) == std::find_if(Content, ArrayEnd(Content), std::bind2nd(std::not_equal_to<uint8_t>(), 0));
    }
  };

  PACK_PRE struct Catalog
  {
    CatEntry Entries[MAX_FILES_COUNT];
    //8
    ServiceSector Meta;
    //9
    Sector Empty;
    //10,11
    Sector CorruptedByMagic[2];
    //12..15
    Sector Empty1[4];
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CatEntry) == 16);
  BOOST_STATIC_ASSERT(sizeof(ServiceSector) == BYTES_PER_SECTOR);
  BOOST_STATIC_ASSERT(sizeof(Sector) == BYTES_PER_SECTOR);
  BOOST_STATIC_ASSERT(sizeof(Catalog) == BYTES_PER_SECTOR * SECTORS_IN_TRACK);

  const Char UNALLOCATED_FILENAME[] = {'$', 'U', 'n', 'a', 'l', 'l', 'o', 'c', 'a', 't', 'e', 'd', 0};

  class Visitor
  {
  public:
    virtual ~Visitor() {}

    virtual void OnFile(const String& name, std::size_t offset, std::size_t size) = 0;
  };

  std::size_t Parse(const Binary::Container& data, Visitor& visitor)
  {
    const std::size_t dataSize = data.Size();
    if (dataSize < sizeof(Catalog) + BYTES_PER_SECTOR)//first track + 1 sector for file at least
    {
      return 0;
    }
    const Catalog* const catalog = safe_ptr_cast<const Catalog*>(data.Data());
    if (!(catalog->Empty.IsEmpty() && 
          catalog->Empty1[0].IsEmpty() &&
          catalog->Empty1[1].IsEmpty() &&
          catalog->Empty1[2].IsEmpty() && 
          catalog->Empty1[3].IsEmpty()))
    {
      Log::Debug(THIS_MODULE, "Invalid track 0 reserved blocks content");
      return 0;
    }
    const bool validSize = dataSize == MODULE_SIZE;

    const std::size_t totalSectors = std::min(dataSize, MODULE_SIZE) / BYTES_PER_SECTOR;
    std::vector<bool> usedSectors(totalSectors);
    std::fill_n(usedSectors.begin(), SECTORS_IN_TRACK, true);
    uint_t files = 0;
    for (const CatEntry* catEntry = catalog->Entries; catEntry != ArrayEnd(catalog->Entries) && NOENTRY != catEntry->Name[0]; ++catEntry)
    {
      if (!catEntry->SizeInSectors)
      {
        continue;
      }
      String entryName = TRDos::GetEntryName(catEntry->Name, catEntry->Type);
      if (DELETED == catEntry->Name[0])
      {
        entryName.insert(0, 1, '~');
      }
      const uint_t offset = SECTORS_IN_TRACK * catEntry->Track + catEntry->Sector;
      const uint_t size = catEntry->SizeInSectors;
      if (offset + size > totalSectors)
      {
        Log::Debug(THIS_MODULE, "File '%1%' is out of bounds", entryName);
        return 0;//out of bounds
      }
      const std::vector<bool>::iterator begin = usedSectors.begin() + offset;
      const std::vector<bool>::iterator end = begin + size;
      if (end != std::find(begin, end, true))
      {
        Log::Debug(THIS_MODULE, "File '%1%' is overlapped with some other", entryName);
        return 0;//overlap
      }
      if (!*(begin - 1))
      {
        Log::Debug(THIS_MODULE, "File '%1%' has a gap before", entryName);
        return 0;//gap
      }
      std::fill(begin, end, true);
      visitor.OnFile(entryName, offset * BYTES_PER_SECTOR, size * BYTES_PER_SECTOR);
      ++files;
    }
    if (!files)
    {
      Log::Debug(THIS_MODULE, "No files in image");
      //no files
      return 0;
    }

    const std::vector<bool>::iterator begin = usedSectors.begin();
    const std::vector<bool>::iterator firstFree = std::find(usedSectors.rbegin(), usedSectors.rend(), true).base();
    const std::vector<bool>::iterator limit = validSize ? usedSectors.end() : firstFree;
    if (validSize && firstFree != limit)
    {
      //do not pay attention to free sector info in service sector
      //std::fill(firstFree, limit, true);
      const std::size_t freeArea = std::distance(begin, firstFree);
      visitor.OnFile(UNALLOCATED_FILENAME, freeArea * BYTES_PER_SECTOR, (totalSectors - freeArea) * BYTES_PER_SECTOR);
    }
    return std::distance(begin, limit) * BYTES_PER_SECTOR;
  }

  class StubVisitor : public Visitor
  {
  public:
    virtual void OnFile(const String& /*filename*/, std::size_t /*offset*/, std::size_t /*size*/) {}
  };

  class BuildVisitorAdapter : public Visitor
  {
  public:
    explicit BuildVisitorAdapter(TRDos::CatalogueBuilder& builder)
      : Builder(builder)
    {
    }

    virtual void OnFile(const String& filename, std::size_t offset, std::size_t size)
    {
      const TRDos::File::Ptr file = TRDos::File::CreateReference(filename, offset, size);
      Builder.AddFile(file);
    }
  private:
    TRDos::CatalogueBuilder& Builder;
  };
}

namespace Formats
{
  namespace Archived
  {
    class TRDDecoder : public Decoder
    {
    public:
      TRDDecoder()
        : Format(Binary::Format::Create(TRD::FORMAT, TRD::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::TRD_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& data) const
      {
        if (!Format->Match(data.Data(), data.Size()))
        {
          return Container::Ptr();
        }
        const TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateFlat();
        TRD::BuildVisitorAdapter visitor(*builder);
        if (const std::size_t size = TRD::Parse(data, visitor))
        {
          builder->SetRawData(data.GetSubcontainer(0, size));
          return builder->GetResult();
        }
        return Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateTRDDecoder()
    {
      return boost::make_shared<TRDDecoder>();
    }
  }
}

