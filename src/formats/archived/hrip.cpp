/**
 *
 * @file
 *
 * @brief  HRiP archives support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/archived/trdos_catalogue.h"
#include "formats/archived/trdos_utils.h"
#include "formats/packed/decoders.h"

#include "binary/format_factories.h"

#include "byteorder.h"
#include "make_ptr.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace Formats::Archived
{
  namespace Hrip
  {
    const auto DESCRIPTION = "Hrip (Hrust RiP archiver)"sv;
    const auto FORMAT =
        "'H'R'i"     // uint8_t ID[3];//'HRi'
        "01-ff"      // uint8_t FilesCount;
        "?"          // uint8_t UsedInLastSector;
        "??"         // uint16_t ArchiveSectors;
        "%0000000x"  // uint8_t Catalogue;
        ""sv;

    const std::size_t MAX_MODULE_SIZE = 655360;

    const uint8_t HRIP_ID[] = {'H', 'R', 'i'};

    struct Header
    {
      uint8_t ID[3];  //'HRi'
      uint8_t FilesCount;
      uint8_t UsedInLastSector;
      le_uint16_t ArchiveSectors;
      uint8_t Catalogue;
    };

    struct BlockHeader
    {
      uint8_t ID[5];  //'Hrst2'
      uint8_t Flag;
      le_uint16_t DataSize;
      le_uint16_t PackedSize;  // without header
      uint8_t AdditionalSize;
      // additional
      le_uint16_t PackedCRC;
      le_uint16_t DataCRC;
      char Name[8];
      char Type[3];
      le_uint16_t Filesize;
      uint8_t Filesectors;
      uint8_t Subdir;
      char Comment[1];

      // flag bits
      enum
      {
        NO_COMPRESSION = 1,
        LAST_BLOCK = 2,
        DELETED = 32
      };
    };

    static_assert(sizeof(Header) * alignof(Header) == 8, "Invalid layout");
    static_assert(offsetof(BlockHeader, PackedCRC) == 11, "Invalid layout");

    // crc16 calculating routine
    inline uint_t CalcCRC(const uint8_t* data, std::size_t size)
    {
      uint_t result = 0;
      while (size--)
      {
        result ^= *data++ << 8;
        for (uint_t i = 0; i < 8; ++i)
        {
          const bool update(0 != (result & 0x8000));
          result <<= 1;
          if (update)
          {
            result ^= 0x1021;
          }
        }
      }
      return result & 0xffff;
    }

    // check archive header and calculate files count and total size
    bool FastCheck(const void* data, std::size_t dataSize, uint_t& files, std::size_t& archiveSize)
    {
      if (dataSize < sizeof(Header))
      {
        return false;
      }
      const auto* const hripHeader = safe_ptr_cast<const Header*>(data);
      if (0 != std::memcmp(hripHeader->ID, HRIP_ID, sizeof(HRIP_ID))
          || (0 != hripHeader->Catalogue && 1 != hripHeader->Catalogue) || !hripHeader->ArchiveSectors
          || !hripHeader->FilesCount)
      {
        return false;
      }
      files = hripHeader->FilesCount;
      if (hripHeader->Catalogue)
      {
        archiveSize = 256 * hripHeader->ArchiveSectors + files * 16 + 6;
      }
      else
      {
        archiveSize = 256 * (hripHeader->ArchiveSectors - 1) + hripHeader->UsedInLastSector;
      }
      return true;
    }

    String ExtractFileName(const void* data)
    {
      const auto* const header = safe_ptr_cast<const BlockHeader*>(data);
      if (header->AdditionalSize >= 18)
      {
        return TRDos::GetEntryName(header->Name, header->Type);
      }
      else
      {
        assert(!"Hrip file without name");
        const auto DEFAULT_HRIP_FILENAME = "NONAME"sv;
        return String{DEFAULT_HRIP_FILENAME};
      }
    }

    Container::Ptr ParseArchive(const Binary::Container& data)
    {
      const std::size_t availSize = data.Size();
      uint_t files = 0;
      std::size_t archiveSize = 0;
      if (!FastCheck(data.Start(), availSize, files, archiveSize))
      {
        return {};
      }
      const TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateGeneric();
      const Formats::Packed::Decoder::Ptr decoder = Packed::CreateHrust23Decoder();
      for (std::size_t rawOffset = sizeof(Header), flatOffset = 0, fileNum = 0; fileNum < files; ++fileNum)
      {
        const std::size_t sourceSize = std::min(MAX_MODULE_SIZE, availSize - rawOffset);
        const Binary::Container::Ptr source = data.GetSubcontainer(rawOffset, sourceSize);
        if (const Formats::Packed::Container::Ptr target = decoder->Decode(*source))
        {
          const String fileName = ExtractFileName(source->Start());
          const std::size_t fileSize = target->Size();
          const std::size_t usedSize = target->PackedSize();
          const TRDos::File::Ptr file = TRDos::File::Create(target, fileName, flatOffset, fileSize);
          builder->AddFile(file);
          rawOffset += usedSize;
          flatOffset += fileSize;
        }
        else
        {
          // TODO: process catalogue
          break;
        }
      }
      builder->SetRawData(data.GetSubcontainer(0, std::min(archiveSize, availSize)));
      return builder->GetResult();
    }
  }  // namespace Hrip

  class HripDecoder : public Decoder
  {
  public:
    HripDecoder()
      : Format(Binary::CreateFormat(Hrip::FORMAT))
    {}

    StringView GetDescription() const override
    {
      return Hrip::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      // implies FastCheck
      return Hrip::ParseArchive(data);
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateHripDecoder()
  {
    return MakePtr<HripDecoder>();
  }
}  // namespace Formats::Archived
