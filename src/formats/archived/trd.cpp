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
    "00-0f"  //free sector
    "01-9f"  //free track
    "16"     //type DS_DD
    "01-7f"  //files
    "?00-09" //free sectors
    "10"     //ID
    "+12+"   //reserved2
    "?"      //deleted files
    "20-7f{8}"//title
  );

  const std::string THIS_MODULE("Formats::Archived::TRD");

  //hints
  const std::size_t MODULE_SIZE = 655360;
  const uint_t BYTES_PER_SECTOR = 256;
  const uint_t SECTORS_IN_TRACK = 16;
  const uint_t MAX_FILES_COUNT = 128;
  const uint_t SERVICE_SECTOR_NUM = 8;

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

  PACK_PRE struct Catalog
  {
    CatEntry Entries[MAX_FILES_COUNT];
    ServiceSector Meta;
    uint8_t Empty[0x700];
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CatEntry) == 16);
  BOOST_STATIC_ASSERT(sizeof(ServiceSector) == 256);
  BOOST_STATIC_ASSERT(sizeof(Catalog) == BYTES_PER_SECTOR * SECTORS_IN_TRACK);

  const Char HIDDEN_FILENAME[] = {'$', 'H', 'i', 'd', 'd', 'e', 'n', 0};
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
    const std::size_t trackSize = SECTORS_IN_TRACK * BYTES_PER_SECTOR;
    const bool validSize = dataSize == MODULE_SIZE;
    const Catalog* const catalog = safe_ptr_cast<const Catalog*>(data.Data());

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
      const uint_t offset = SECTORS_IN_TRACK * catEntry->Track + catEntry->Sector;
      const uint_t size = catEntry->SizeInSectors;
      if (offset + size > totalSectors)
      {
        return 0;//out of bounds
      }
      const std::vector<bool>::iterator begin = usedSectors.begin() + offset;
      const std::vector<bool>::iterator end = begin + size;
      if (end != std::find(begin, end, true))
      {
        return 0;//overlap
      }
      std::fill(begin, end, true);
      String entryName = TRDos::GetEntryName(catEntry->Name, catEntry->Type);
      if (DELETED == catEntry->Name[0])
      {
        entryName.insert(0, 1, '~');
      }
      visitor.OnFile(entryName, offset * BYTES_PER_SECTOR, size * BYTES_PER_SECTOR);
      ++files;
    }
    if (!files)
    {
      //no files
      return 0;
    }
    if (ArrayEnd(catalog->Empty) != std::find_if(catalog->Empty, ArrayEnd(catalog->Empty), std::bind1st(std::not_equal_to<uint8_t>(), 0)))
    {
      return 0;//not empty
    }

    const std::vector<bool>::iterator begin = usedSectors.begin();
    const std::vector<bool>::iterator limit = validSize ? usedSectors.end() : std::find(usedSectors.rbegin(), usedSectors.rend(), true).base();
    if (validSize)
    {
      const std::size_t freeArea = (SECTORS_IN_TRACK * catalog->Meta.FreeSpaceTrack + catalog->Meta.FreeSpaceSect);
      if (freeArea > SECTORS_IN_TRACK && freeArea < totalSectors)
      {
        const std::vector<bool>::iterator freeBegin = usedSectors.begin() + freeArea;
        if (limit == std::find(freeBegin, limit, true))
        {
          std::fill(freeBegin, limit, true);
          visitor.OnFile(UNALLOCATED_FILENAME, freeArea * BYTES_PER_SECTOR, (totalSectors - freeArea) * BYTES_PER_SECTOR);
        }
      }
    }
    for (std::vector<bool>::iterator empty = std::find(begin, limit, false); 
         empty != limit;
         )
    {
      const std::vector<bool>::iterator emptyEnd = std::find(empty, limit, true);
      const std::size_t offset = BYTES_PER_SECTOR * (empty - begin);
      const std::size_t size = BYTES_PER_SECTOR * (emptyEnd - empty);
      visitor.OnFile(HIDDEN_FILENAME, offset, size);
      empty = std::find(emptyEnd, limit, false);
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
        : Format(Binary::Format::Create(TRD::FORMAT))
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

