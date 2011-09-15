/*
Abstract:
  SCL containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "trdos_catalogue.h"
#include "trdos_utils.h"
#include "core/plugins/registrator.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
//std includes
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const std::size_t BYTES_PER_SECTOR = 256;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct SCLEntry
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

  PACK_PRE struct SCLHeader
  {
    uint8_t ID[8];//'SINCLAIR'
    uint8_t BlocksCount;
    SCLEntry Blocks[1];
  };
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t SINCLAIR_ID[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R'};

  //header with one entry + one sector + CRC
  const std::size_t SCL_MIN_SIZE = sizeof(SCLHeader) + BYTES_PER_SECTOR + 4;
  //header + up to 255 entries for 255 sectors each
  const std::size_t SCL_MODULE_SIZE = sizeof(SCLHeader) - sizeof(SCLEntry) +
    255 * (sizeof(SCLEntry) + 0xff * BYTES_PER_SECTOR) + 4;

  BOOST_STATIC_ASSERT(sizeof(SCLEntry) == 14);
  BOOST_STATIC_ASSERT(sizeof(SCLHeader) == 23);

  uint_t SumDataSize(uint_t prevSize, const SCLEntry& entry)
  {
    return prevSize + entry.Size();
  }

  bool CheckSCLFile(const IO::DataContainer& data)
  {
    const std::size_t limit = data.Size();
    if (limit < SCL_MIN_SIZE)
    {
      return false;
    }
    const SCLHeader* const header = safe_ptr_cast<const SCLHeader*>(data.Data());
    if (0 != std::memcmp(header->ID, SINCLAIR_ID, sizeof(SINCLAIR_ID)) ||
        0 == header->BlocksCount)
    {
      return false;
    }
    const std::size_t descriptionsSize = sizeof(*header) + sizeof(header->Blocks) * (header->BlocksCount - 1);
    if (descriptionsSize > limit)
    {
      return false;
    }
    const std::size_t dataSize = std::accumulate(header->Blocks, header->Blocks + header->BlocksCount, 0, &SumDataSize);
    if (descriptionsSize + dataSize + sizeof(uint32_t) > limit)
    {
      return false;
    }
    const std::size_t checksumOffset = descriptionsSize + dataSize;
    const uint8_t* const dump = static_cast<const uint8_t*>(data.Data());
    const uint32_t storedChecksum = fromLE(*safe_ptr_cast<const uint32_t*>(dump + checksumOffset));
    const uint32_t checksum = std::accumulate(dump, dump + checksumOffset, uint32_t(0));
    return storedChecksum == checksum;
  }

  //fill descriptors array and return actual container size
  Container::Catalogue::Ptr ParseSCLFile(IO::DataContainer::Ptr data)
  {
    if (!CheckSCLFile(*data))
    {
      return Container::Catalogue::Ptr();
    }
    const SCLHeader* const header = safe_ptr_cast<const SCLHeader*>(data->Data());

    const TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateFlat(data);
    std::size_t offset = safe_ptr_cast<const uint8_t*>(header->Blocks + header->BlocksCount) -
                    safe_ptr_cast<const uint8_t*>(header);
    for (uint_t idx = 0; idx != header->BlocksCount; ++idx)
    {
      const SCLEntry& entry = header->Blocks[idx];
      const std::size_t nextOffset = offset + entry.Size();
      const String entryName = TRDos::GetEntryName(entry.Name, entry.Type);
      const TRDos::File::Ptr newOne = TRDos::File::CreateReference(entryName, offset, entry.Size());
      builder->AddFile(newOne);
      offset = nextOffset;
    }
    builder->SetUsedSize(offset);
    return builder->GetResult();
  }
}

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'S', 'C', 'L', 0};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::SCL_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK | CAP_STOR_PLAIN;

  const std::string SCL_FORMAT(
    "'S'I'N'C'L'A'I'R"
    "01-ff"
  );

  class SCLFactory : public ContainerFactory
  {
  public:
    SCLFactory()
      : Format(Binary::Format::Create(SCL_FORMAT))
    {
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Container::Catalogue::Ptr CreateContainer(const Parameters::Accessor& /*parameters*/, IO::DataContainer::Ptr data) const
    {
      return ParseSCLFile(data);
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterSCLContainer(PluginsRegistrator& registrator)
  {
    const ContainerFactory::Ptr factory = boost::make_shared<SCLFactory>();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
