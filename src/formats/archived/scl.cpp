/**
 *
 * @file
 *
 * @brief  SCL images support
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
#include <pointers.h>
// library includes
#include <binary/format_factories.h>
#include <debug/log.h>
// std includes
#include <cstring>
#include <numeric>

namespace Formats::Archived
{
  namespace SCL
  {
    const Debug::Stream Dbg("Formats::Archived::SCL");

    const auto DESCRIPTION = "SCL (SINCLAIR)"sv;
    const auto FORMAT =
        "'S'I'N'C'L'A'I'R"
        "01-ff"
        ""sv;

    const std::size_t BYTES_PER_SECTOR = 256;

    struct Entry
    {
      char Name[8];
      char Type[3];
      le_uint16_t Length;
      uint8_t SizeInSectors;

      uint_t Size() const
      {
        // use rounded file size for better compatibility
        return BYTES_PER_SECTOR * SizeInSectors;
      }
    };

    struct Header
    {
      uint8_t ID[8];  //'SINCLAIR'
      uint8_t BlocksCount;
      Entry Blocks[1];
    };

    const uint8_t SIGNATURE[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R'};

    // header with one entry + one sector + CRC
    const std::size_t MIN_SIZE = sizeof(Header) + BYTES_PER_SECTOR + 4;

    static_assert(sizeof(Entry) * alignof(Entry) == 14, "Invalid layout");
    static_assert(sizeof(Header) * alignof(Header) == 23, "Invalid layout");

    uint_t SumDataSize(uint_t prevSize, const Entry& entry)
    {
      return prevSize + entry.Size();
    }

    bool FastCheck(Binary::View data)
    {
      const std::size_t limit = data.Size();
      if (limit < MIN_SIZE)
      {
        return false;
      }
      const auto* header = data.As<Header>();
      if (0 != std::memcmp(header->ID, SIGNATURE, sizeof(SIGNATURE)) || 0 == header->BlocksCount)
      {
        return false;
      }
      const std::size_t descriptionsSize = sizeof(*header) + sizeof(header->Blocks) * (header->BlocksCount - 1);
      if (descriptionsSize > limit)
      {
        Dbg("No place for data at all");
        return false;
      }
      const std::size_t dataSize =
          std::accumulate(header->Blocks, header->Blocks + header->BlocksCount, 0, &SumDataSize);
      if (descriptionsSize + dataSize + sizeof(uint32_t) > limit)
      {
        Dbg("No place for all data");
        return false;
      }
      const std::size_t checksumOffset = descriptionsSize + dataSize;
      const auto* dump = data.As<uint8_t>();
      const auto storedChecksum = ReadLE<uint32_t>(dump + checksumOffset);
      const uint32_t checksum = std::accumulate(dump, dump + checksumOffset, uint32_t(0));
      if (storedChecksum != checksum)
      {
        Dbg("Invalid checksum (stored {}@{}, calculated {})", storedChecksum, checksumOffset, checksum);
        return false;
      }
      return true;
    }

    // fill descriptors array and return actual container size
    Container::Ptr ParseArchive(const Binary::Container& rawData)
    {
      const Binary::View data(rawData);
      if (!FastCheck(data))
      {
        return {};
      }
      const auto* header = data.As<Header>();

      const auto builder = TRDos::CatalogueBuilder::CreateFlat();
      std::size_t offset =
          safe_ptr_cast<const uint8_t*>(header->Blocks + header->BlocksCount) - safe_ptr_cast<const uint8_t*>(header);
      for (uint_t idx = 0; idx != header->BlocksCount; ++idx)
      {
        const Entry& entry = header->Blocks[idx];
        const std::size_t nextOffset = offset + entry.Size();
        const String entryName = TRDos::GetEntryName(entry.Name, entry.Type);
        auto newOne = TRDos::File::CreateReference(entryName, offset, entry.Size());
        builder->AddFile(std::move(newOne));
        offset = nextOffset;
      }
      // use checksum
      offset += sizeof(uint32_t);
      builder->SetRawData(rawData.GetSubcontainer(0, offset));
      return builder->GetResult();
    }
  }  // namespace SCL

  class SCLDecoder : public Decoder
  {
  public:
    SCLDecoder()
      : Format(Binary::CreateFormat(SCL::FORMAT, SCL::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return SCL::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& data) const override
    {
      // implies SCL::FastCheck
      return SCL::ParseArchive(data);
    }

  private:
    const Binary::Format::Ptr Format;
  };

  Decoder::Ptr CreateSCLDecoder()
  {
    return MakePtr<SCLDecoder>();
  }
}  // namespace Formats::Archived
