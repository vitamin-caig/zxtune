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
    "+2048+"//skip to service sector
    "+227+"//skip zeroes in service sector
    "16"            //type DS_DD
    "?"             //files
    "?%00000xxx"    //free sectors
    "10"            //ID
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

    uint_t Offset() const
    {
      return BYTES_PER_SECTOR * (SECTORS_IN_TRACK * Track + Sector);
    }

    uint_t Size() const
    {
      //use rounded file size for better compatibility
      return BYTES_PER_SECTOR * SizeInSectors;
    }
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

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CatEntry) == 16);
  BOOST_STATIC_ASSERT(sizeof(ServiceSector) == 256);

  bool FastCheck(const Binary::Container& data)
  {
    //it's meaningless to support trunkated files
    if (data.Size() < MODULE_SIZE)
    {
      return false;
    }
    const ServiceSector* const sector = safe_ptr_cast<const ServiceSector*>(data.Data()) + SERVICE_SECTOR_NUM;
    if (sector->ID != TRDOS_ID || sector->Type != DS_DD || 0 != sector->Zero)
    {
      return false;
    }
    const CatEntry* catEntry = safe_ptr_cast<const CatEntry*>(data.Data());
    const RangeChecker::Ptr checker = RangeChecker::Create(MODULE_SIZE);
    checker->AddRange(0, SECTORS_IN_TRACK * BYTES_PER_SECTOR);
    uint_t idx = 0;
    for (; idx != MAX_FILES_COUNT && NOENTRY != catEntry->Name[0]; ++idx, ++catEntry)
    {
      if (!catEntry->SizeInSectors)
      {
        continue;
      }
      const uint_t offset = catEntry->Offset();
      const uint_t size = catEntry->Size();
      if (!checker->AddRange(offset, size))
      {
        return false;
      }
    }
    return idx > 0;
  }

  Archived::Container::Ptr ParseArchive(const Binary::Container& data)
  {
    if (!FastCheck(data))
    {
      return Archived::Container::Ptr();
    }
    const TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateFlat();

    const ServiceSector* const sector = safe_ptr_cast<const ServiceSector*>(data.Data()) + SERVICE_SECTOR_NUM;
    uint_t deleted = 0;
    const CatEntry* catEntry = safe_ptr_cast<const CatEntry*>(data.Data());
    for (uint_t idx = 0; idx != MAX_FILES_COUNT && NOENTRY != catEntry->Name[0]; ++idx, ++catEntry)
    {
      //TODO: parametrize this
      const bool isDeleted = DELETED == catEntry->Name[0];
      if (isDeleted)
      {
        ++deleted;
      }
      if (catEntry->SizeInSectors)
      {
        String entryName = TRDos::GetEntryName(catEntry->Name, catEntry->Type);
        if (isDeleted)
        {
          entryName.insert(0, 1, '~');
        }
        const TRDos::File::Ptr newOne = TRDos::File::CreateReference(entryName, catEntry->Offset(), catEntry->Size());
        builder->AddFile(newOne);
      }
    }
    if (deleted != sector->DeletedFiles)
    {
      Log::Debug(THIS_MODULE, "Deleted files count is differs from calculated");
    }
    builder->SetRawData(data.GetSubcontainer(0, MODULE_SIZE));
    return builder->GetResult();
  }
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

      virtual bool Check(const Binary::Container& data) const
      {
        return TRD::FastCheck(data);
      }

      virtual Container::Ptr Decode(const Binary::Container& data) const
      {
        return TRD::ParseArchive(data);
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

