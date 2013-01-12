/*
Abstract:
  SCL containers support

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
#include <tools.h>
//library includes
#include <debug/log.h>
//std includes
#include <cstring>
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>
//text include
#include <formats/text/archived.h>

namespace SCL
{
  using namespace Formats;

  const std::string FORMAT(
    "'S'I'N'C'L'A'I'R"
    "01-ff"
  );

  const Debug::Stream Dbg("Formats::Archived::SCL");

  const std::size_t BYTES_PER_SECTOR = 256;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Entry
  {
    char Name[8];
    char Type[3];
    uint16_t Length;
    uint8_t SizeInSectors;

    uint_t Size() const
    {
      //use rounded file size for better compatibility
      return BYTES_PER_SECTOR * SizeInSectors;
    }
  } PACK_POST;

  PACK_PRE struct Header
  {
    uint8_t ID[8];//'SINCLAIR'
    uint8_t BlocksCount;
    Entry Blocks[1];
  };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t SIGNATURE[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R'};

  //header with one entry + one sector + CRC
  const std::size_t MIN_SIZE = sizeof(Header) + BYTES_PER_SECTOR + 4;
  //header + up to 255 entries for 255 sectors each
  const std::size_t MODULE_SIZE = sizeof(Header) - sizeof(Entry) +
    255 * (sizeof(Entry) + 0xff * BYTES_PER_SECTOR) + 4;

  BOOST_STATIC_ASSERT(sizeof(Entry) == 14);
  BOOST_STATIC_ASSERT(sizeof(Header) == 23);

  uint_t SumDataSize(uint_t prevSize, const Entry& entry)
  {
    return prevSize + entry.Size();
  }

  bool FastCheck(const Binary::Container& data)
  {
    const std::size_t limit = data.Size();
    if (limit < MIN_SIZE)
    {
      return false;
    }
    const Header* const header = safe_ptr_cast<const Header*>(data.Start());
    if (0 != std::memcmp(header->ID, SIGNATURE, sizeof(SIGNATURE)) ||
        0 == header->BlocksCount)
    {
      return false;
    }
    const std::size_t descriptionsSize = sizeof(*header) + sizeof(header->Blocks) * (header->BlocksCount - 1);
    if (descriptionsSize > limit)
    {
      Dbg("No place for data at all");
      return false;
    }
    const std::size_t dataSize = std::accumulate(header->Blocks, header->Blocks + header->BlocksCount, 0, &SumDataSize);
    if (descriptionsSize + dataSize + sizeof(uint32_t) > limit)
    {
      Dbg("No place for all data");
      return false;
    }
    const std::size_t checksumOffset = descriptionsSize + dataSize;
    const uint8_t* const dump = static_cast<const uint8_t*>(data.Start());
    const uint32_t storedChecksum = fromLE(*safe_ptr_cast<const uint32_t*>(dump + checksumOffset));
    const uint32_t checksum = std::accumulate(dump, dump + checksumOffset, uint32_t(0));
    if (storedChecksum != checksum)
    {
      Dbg("Invalid checksum (stored %1%@%2%, calculated %3%)", storedChecksum, checksumOffset, checksum);
      return false;
    }
    return true;
  }

  //fill descriptors array and return actual container size
  Archived::Container::Ptr ParseArchive(const Binary::Container& data)
  {
    if (!FastCheck(data))
    {
      return Archived::Container::Ptr();
    }
    const Header* const header = safe_ptr_cast<const Header*>(data.Start());

    const TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateFlat();
    std::size_t offset = safe_ptr_cast<const uint8_t*>(header->Blocks + header->BlocksCount) -
                    safe_ptr_cast<const uint8_t*>(header);
    for (uint_t idx = 0; idx != header->BlocksCount; ++idx)
    {
      const Entry& entry = header->Blocks[idx];
      const std::size_t nextOffset = offset + entry.Size();
      const String entryName = TRDos::GetEntryName(entry.Name, entry.Type);
      const TRDos::File::Ptr newOne = TRDos::File::CreateReference(entryName, offset, entry.Size());
      builder->AddFile(newOne);
      offset = nextOffset;
    }
    //use checksum
    offset += sizeof(uint32_t);
    builder->SetRawData(data.GetSubcontainer(0, offset));
    return builder->GetResult();
  }
}

namespace Formats
{
  namespace Archived
  {
    class SCLDecoder : public Decoder
    {
    public:
      SCLDecoder()
        : Format(Binary::Format::Create(SCL::FORMAT, SCL::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SCL_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& data) const
      {
        //implies SCL::FastCheck
        return SCL::ParseArchive(data);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateSCLDecoder()
    {
      return boost::make_shared<SCLDecoder>();
    }
  }
}

