/**
 *
 * @file
 *
 * @brief  TRD images support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/trdos_catalogue.h"
#include "formats/archived/trdos_utils.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
#include <range_checker.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
// std includes
#include <cstring>
#include <numeric>

namespace Formats::Archived
{
  namespace TRD
  {
    const Debug::Stream Dbg("Formats::Archived::TRD");

    const Char DESCRIPTION[] = "TRD (TR-DOS)";
    const auto FORMAT =
        "(00|01|20-7f??????? ??? ?? ? 0x 00-a0){128}"
        // service sector
        "00"               // zero
        "?{224}"           // reserved
        "?"                // free sector
        "?"                // free track
        "16"               // type DS_DD
        "01-7f"            // files
        "?00-09"           // free sectors
        "10"               // ID
        "0000?????????00"  // reserved
        "?"                // deleted files
        "20-7f{8}"         // title
        "000000"           // reserved
        ""sv;

    // hints
    const std::size_t MODULE_SIZE = 655360;
    const uint_t BYTES_PER_SECTOR = 256;
    const uint_t SECTORS_IN_TRACK = 16;
    const uint_t MAX_FILES_COUNT = 128;
    const std::size_t MIN_SIZE = (SECTORS_IN_TRACK + 1) * BYTES_PER_SECTOR;

    enum
    {
      NOENTRY = 0,
      DELETED = 1
    };

    struct CatEntry
    {
      char Name[8];
      char Type[3];
      le_uint16_t Length;
      uint8_t SizeInSectors;
      uint8_t Sector;
      uint8_t Track;
    };

    enum
    {
      TRDOS_ID = 0x10,

      DS_DD = 0x16,
      DS_SD = 0x17,
      SS_DD = 0x18,
      SS_SD = 0x19
    };

    struct ServiceSector
    {
      uint8_t Zero;
      uint8_t Reserved1[224];
      uint8_t FreeSpaceSect;
      uint8_t FreeSpaceTrack;
      uint8_t Type;
      uint8_t Files;
      le_uint16_t FreeSectors;
      uint8_t ID;  // 0x10
      uint8_t Reserved2[12];
      uint8_t DeletedFiles;
      uint8_t Title[8];
      uint8_t Reserved3[3];
    };

    struct Sector
    {
      uint8_t Content[BYTES_PER_SECTOR];

      bool IsEmpty() const
      {
        return std::all_of(Content, std::end(Content), [](auto b) { return b == 0; });
      }
    };

    struct Catalog
    {
      CatEntry Entries[MAX_FILES_COUNT];
      // 8
      ServiceSector Meta;
      // 9
      Sector Empty;
      // 10,11
      Sector CorruptedByMagic[2];
      // 12..15
      Sector Empty1[4];
    };

    static_assert(sizeof(CatEntry) * alignof(CatEntry) == 16, "Invalid layout");
    static_assert(sizeof(ServiceSector) * alignof(ServiceSector) == BYTES_PER_SECTOR, "Invalid layout");
    static_assert(sizeof(Sector) * alignof(Sector) == BYTES_PER_SECTOR, "Invalid layout");
    static_assert(sizeof(Catalog) * alignof(Catalog) == BYTES_PER_SECTOR * SECTORS_IN_TRACK, "Invalid layout");

    const Char UNALLOCATED_FILENAME[] = {'$', 'U', 'n', 'a', 'l', 'l', 'o', 'c', 'a', 't', 'e', 'd', 0};

    class Visitor
    {
    public:
      virtual ~Visitor() = default;

      virtual void OnFile(StringView name, std::size_t offset, std::size_t size) = 0;
    };

    std::size_t Parse(Binary::View data, Visitor& visitor)
    {
      const std::size_t dataSize = data.Size();
      if (dataSize < sizeof(Catalog) + BYTES_PER_SECTOR)  // first track + 1 sector for file at least
      {
        return 0;
      }
      const auto* catalog = data.As<Catalog>();
      if (!(catalog->Empty.IsEmpty() && catalog->Empty1[0].IsEmpty() && catalog->Empty1[1].IsEmpty()
            && catalog->Empty1[2].IsEmpty() && catalog->Empty1[3].IsEmpty()))
      {
        Dbg("Invalid track 0 reserved blocks content");
        return 0;
      }
      const bool validSize = dataSize == MODULE_SIZE;

      const std::size_t totalSectors = std::min(dataSize, MODULE_SIZE) / BYTES_PER_SECTOR;
      std::vector<bool> usedSectors(totalSectors);
      std::fill_n(usedSectors.begin(), SECTORS_IN_TRACK, true);
      uint_t files = 0;
      for (const auto* catEntry = catalog->Entries;
           catEntry != std::end(catalog->Entries) && NOENTRY != catEntry->Name[0]; ++catEntry)
      {
        if (!catEntry->SizeInSectors)
        {
          continue;
        }
        auto entryName = TRDos::GetEntryName(catEntry->Name, catEntry->Type);
        if (DELETED == catEntry->Name[0])
        {
          entryName.insert(0, 1, '~');
        }
        const uint_t offset = SECTORS_IN_TRACK * catEntry->Track + catEntry->Sector;
        const uint_t size = catEntry->SizeInSectors;
        if (offset + size > totalSectors)
        {
          Dbg("File '{}' is out of bounds", entryName);
          return 0;  // out of bounds
        }
        const auto begin = usedSectors.begin() + offset;
        const auto end = begin + size;
        if (end != std::find(begin, end, true))
        {
          Dbg("File '{}' is overlapped with some other", entryName);
          return 0;  // overlap
        }
        if (!*(begin - 1))
        {
          Dbg("File '{}' has a gap before", entryName);
          return 0;  // gap
        }
        std::fill(begin, end, true);
        visitor.OnFile(entryName, offset * BYTES_PER_SECTOR, size * BYTES_PER_SECTOR);
        ++files;
      }
      if (!files)
      {
        Dbg("No files in image");
        // no files
        return 0;
      }

      const auto begin = usedSectors.begin();
      const auto firstFree = std::find(usedSectors.rbegin(), usedSectors.rend(), true).base();
      const auto limit = validSize ? usedSectors.end() : firstFree;
      if (validSize && firstFree != limit)
      {
        // do not pay attention to free sector info in service sector
        // std::fill(firstFree, limit, true);
        const std::size_t freeArea = std::distance(begin, firstFree);
        visitor.OnFile(UNALLOCATED_FILENAME, freeArea * BYTES_PER_SECTOR, (totalSectors - freeArea) * BYTES_PER_SECTOR);
      }
      return std::distance(begin, limit) * BYTES_PER_SECTOR;
    }

    class BuildVisitorAdapter : public Visitor
    {
    public:
      explicit BuildVisitorAdapter(TRDos::CatalogueBuilder& builder)
        : Builder(builder)
      {}

      void OnFile(StringView filename, std::size_t offset, std::size_t size) override
      {
        auto file = TRDos::File::CreateReference(filename, offset, size);
        Builder.AddFile(std::move(file));
      }

    private:
      TRDos::CatalogueBuilder& Builder;
    };
  }  // namespace TRD

  class TRDDecoder : public Decoder
  {
  public:
    TRDDecoder()
      : Format(Binary::CreateFormat(TRD::FORMAT, TRD::MIN_SIZE))
    {}

    String GetDescription() const override
    {
      return TRD::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      const Binary::View data(rawData);
      if (!Format->Match(data))
      {
        return {};
      }
      const auto builder = TRDos::CatalogueBuilder::CreateFlat();
      TRD::BuildVisitorAdapter visitor(*builder);
      if (const std::size_t size = TRD::Parse(data, visitor))
      {
        builder->SetRawData(rawData.GetSubcontainer(0, size));
        return builder->GetResult();
      }
      return {};
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateTRDDecoder()
  {
    return MakePtr<TRDDecoder>();
  }
}  // namespace Formats::Archived
