/*
Abstract:
  HRiP containers support

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
#include "core/src/core.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <parameters.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <formats/packed_decoders.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 942A64CC

namespace
{
  using namespace ZXTune;

  const std::size_t HRP_MODULE_SIZE = 655360;

  const uint8_t HRIP_ID[] = {'H', 'R', 'i'};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct HripHeader
  {
    uint8_t ID[3];//'HRi'
    uint8_t FilesCount;
    uint8_t UsedInLastSector;
    uint16_t ArchiveSectors;
    uint8_t Catalogue;
  } PACK_POST;

  PACK_PRE struct HripBlockHeader
  {
    uint8_t ID[5];//'Hrst2'
    uint8_t Flag;
    uint16_t DataSize;
    uint16_t PackedSize;//without header
    uint8_t AdditionalSize;
    //additional
    uint16_t PackedCRC;
    uint16_t DataCRC;
    char Name[8];
    char Type[3];
    uint16_t Filesize;
    uint8_t Filesectors;
    uint8_t Subdir;
    char Comment[1];

    //flag bits
    enum
    {
      NO_COMPRESSION = 1,
      LAST_BLOCK = 2,
      DELETED = 32
    };
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(HripHeader) == 8);
  BOOST_STATIC_ASSERT(offsetof(HripBlockHeader, PackedCRC) == 11);

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

  //check archive header and calculate files count and total size
  bool CheckHrip(const void* data, std::size_t dataSize, uint_t& files, std::size_t& archiveSize)
  {
    if (dataSize < sizeof(HripHeader))
    {
      return false;
    }
    const HripHeader* const hripHeader = static_cast<const HripHeader*>(data);
    if (0 != std::memcmp(hripHeader->ID, HRIP_ID, sizeof(HRIP_ID)) ||
       !(0 == hripHeader->Catalogue || 1 == hripHeader->Catalogue) ||
       !hripHeader->ArchiveSectors || !hripHeader->FilesCount)
    {
      return false;
    }
    files = hripHeader->FilesCount;
    if (hripHeader->Catalogue)
    {
      archiveSize = 256 * fromLE(hripHeader->ArchiveSectors) + files * 16 + 6;
    }
    else
    {
      archiveSize = 256 * (fromLE(hripHeader->ArchiveSectors) - 1) + hripHeader->UsedInLastSector;
    }
    return true;
  }

  inline bool CheckIgnoreCorrupted(const Parameters::Accessor& params)
  {
    Parameters::IntType val;
    return params.FindIntValue(Parameters::ZXTune::Core::Plugins::Hrip::IGNORE_CORRUPTED, val) && val != 0;
  }

  String ExtractFileName(const void* data)
  {
    const HripBlockHeader* const header = static_cast<const HripBlockHeader*>(data);
    if (header->AdditionalSize >= 18)
    {
      return TRDos::GetEntryName(header->Name, header->Type);
    }
    else
    {
      assert(!"Hrip file without name");
      static const Char DEFAULT_HRIP_FILENAME[] = {'N','O','N','A','M','E',0};
      return DEFAULT_HRIP_FILENAME;
    }
  }

  Container::Catalogue::Ptr ParseHripFile(const Binary::Container& data, const Parameters::Accessor& params)
  {
    uint_t files = 0;
    std::size_t archiveSize = 0;
    const uint8_t* const ptr = static_cast<const uint8_t*>(data.Data());
    const std::size_t size = data.Size();
    if (!CheckHrip(ptr, size, files, archiveSize))
    {
      return Container::Catalogue::Ptr();
    }
    const bool ignoreCorrupted = CheckIgnoreCorrupted(params);

    const TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateGeneric();
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateHrust23Decoder();
    for (std::size_t rawOffset = sizeof(HripHeader), flatOffset = 0, fileNum = 0; fileNum < files; ++fileNum)
    {
      const Binary::Container::Ptr source = data.GetSubcontainer(rawOffset, size);
      if (const Formats::Packed::Container::Ptr target = decoder->Decode(*source))
      {
        const String fileName = ExtractFileName(source->Data());
        const std::size_t fileSize = target->Size();
        const std::size_t usedSize = target->PackedSize();
        const TRDos::File::Ptr file = TRDos::File::Create(target, fileName, flatOffset, fileSize);
        builder->AddFile(file);
        rawOffset += usedSize;
        flatOffset += fileSize;
      }
      else
      {
        //TODO: process catalogue
        break;
      }
    }
    builder->SetUsedSize(archiveSize);
    return builder->GetResult();
  }
}

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'H', 'R', 'I', 'P', 0};
  const Char* const INFO = Text::HRIP_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK;

  const std::string HRIP_FORMAT(
    "'H'R'i" //uint8_t ID[3];//'HRi'
    "01-ff"  //uint8_t FilesCount;
    "?"      //uint8_t UsedInLastSector;
    "??"     //uint16_t ArchiveSectors;
    "%0000000x"//uint8_t Catalogue;
  );

  class HRIPFactory : public ContainerFactory
  {
  public:
    HRIPFactory()
      : Format(Binary::Format::Create(HRIP_FORMAT))
    {
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Container::Catalogue::Ptr CreateContainer(const Parameters::Accessor& parameters, Binary::Container::Ptr data) const
    {
      return ParseHripFile(*data, parameters);
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterHRIPContainer(PluginsRegistrator& registrator)
  {
    const ContainerFactory::Ptr factory = boost::make_shared<HRIPFactory>();
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
