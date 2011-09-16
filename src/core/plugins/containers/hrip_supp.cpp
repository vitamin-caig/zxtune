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

  const uint8_t HRIP_BLOCK_ID[] = {'H', 'r', 's', 't', '2'};
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
  BOOST_STATIC_ASSERT(sizeof(HripBlockHeader) == 29);

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

  class DataSource
  {
  public:
    typedef boost::shared_ptr<const DataSource> Ptr;

    explicit DataSource(Binary::Container::Ptr data)
      : Decoder(Formats::Packed::CreateHrust2RawDecoder())
      , Data(data)
    {
    }

    Binary::Container::Ptr DecodeBlock(const HripBlockHeader* block) const
    {
      const uint8_t* const packedData = safe_ptr_cast<const uint8_t*>(&block->PackedCRC) + block->AdditionalSize;
      const std::size_t packedSize = fromLE(block->PackedSize);
      if (0 != (block->Flag & HripBlockHeader::NO_COMPRESSION))
      {
        const std::size_t offset = packedData - static_cast<const uint8_t*>(Data->Data());
        return Data->GetSubcontainer(offset, packedSize);
      }
      else if (Decoder->Check(packedData, packedSize))
      {
        return Decoder->Decode(packedData, packedSize);
      }
      return Binary::Container::Ptr();
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
    const Binary::Container::Ptr Data;
  };

  typedef std::vector<const HripBlockHeader*> HripBlockHeadersList;

  class HripFile : public TRDos::File
  {
  public:
    HripFile(DataSource::Ptr data, std::size_t offset, const HripBlockHeadersList& blocks, bool ignoreCorrupted)
      : Data(data)
      , Offset(offset)
      , Blocks(blocks)
      , FirstBlock(*Blocks.front())
      , IgnoreCorrupted(ignoreCorrupted)
    {
    }

    virtual String GetName() const
    {
      if (FirstBlock.AdditionalSize > 15)
      {
        return TRDos::GetEntryName(FirstBlock.Name, FirstBlock.Type);
      }
      else
      {
        assert(!"Hrip file without name");
        static const Char DEFAULT_HRIP_FILENAME[] = {'N','O','N','A','M','E',0};
        return DEFAULT_HRIP_FILENAME;
      }
    }

    virtual std::size_t GetOffset() const
    {
      return Offset;
    }

    virtual std::size_t GetSize() const
    {
      return std::accumulate(Blocks.begin(), Blocks.end(), std::size_t(0), 
        boost::bind(std::plus<std::size_t>(), _1,
          boost::bind(&fromLE<uint16_t>, boost::bind(&HripBlockHeader::DataSize, _2))));
    }

    virtual Binary::Container::Ptr GetData() const
    {
      std::auto_ptr<Dump> result(new Dump(GetSize()));
      Dump::iterator dst = result->begin();
      for (HripBlockHeadersList::const_iterator it = Blocks.begin(), lim = Blocks.end(); it != lim; ++it)
      {
        const HripBlockHeader* const block = *it;
        const uint8_t* const packedData = safe_ptr_cast<const uint8_t*>(&block->PackedCRC) + block->AdditionalSize;
        const std::size_t packedSize = fromLE(block->PackedSize);
        if (!IgnoreCorrupted &&
            (block->AdditionalSize >= 2 && fromLE(block->PackedCRC) != CalcCRC(packedData, packedSize)))
        {
          return Binary::Container::Ptr();
        }
        if (Binary::Container::Ptr decodedBlock = Data->DecodeBlock(block))
        {
          const uint8_t* const blockData = static_cast<const uint8_t*>(decodedBlock->Data());
          const std::size_t blockSize = decodedBlock->Size();
          if (!IgnoreCorrupted &&
              (block->AdditionalSize >= 4 && fromLE(block->DataCRC) != CalcCRC(blockData, blockSize)))
          {
            return Binary::Container::Ptr();
          }
          //single block
          if (1 == Blocks.size())
          {
            return decodedBlock;
          }
          dst = std::copy(blockData, blockData + blockSize, dst);
        }
        else if (!IgnoreCorrupted)
        {
          return Binary::Container::Ptr();
        }
      }
      return Binary::CreateContainer(result);
    }
  private:
    const DataSource::Ptr Data;
    const std::size_t Offset;
    const HripBlockHeadersList Blocks;
    const HripBlockHeader& FirstBlock;
    const bool IgnoreCorrupted;
  };

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


  Container::Catalogue::Ptr ParseHripFile(Binary::Container::Ptr data, const Parameters::Accessor& params)
  {
    uint_t files = 0;
    std::size_t archiveSize = 0;
    const uint8_t* const ptr = static_cast<const uint8_t*>(data->Data());
    const std::size_t size = data->Size();
    if (!CheckHrip(ptr, size, files, archiveSize))
    {
      return Container::Catalogue::Ptr();
    }
    const bool ignoreCorrupted = CheckIgnoreCorrupted(params);

    const TRDos::CatalogueBuilder::Ptr builder = TRDos::CatalogueBuilder::CreateGeneric();
    const DataSource::Ptr source = boost::make_shared<DataSource>(data);

    std::size_t flatOffset = 0;
    std::size_t offset = sizeof(HripHeader);
    for (uint_t fileNum = 0; fileNum < files; ++fileNum)
    {
      //collect file's blocks
      HripBlockHeadersList blocks;
      for (;;)
      {
        const HripBlockHeader* const blockHdr = safe_ptr_cast<const HripBlockHeader*>(ptr + offset);
        const uint8_t* const packedData = safe_ptr_cast<const uint8_t*>(&blockHdr->PackedCRC) +
          blockHdr->AdditionalSize;
        if (0 != std::memcmp(blockHdr->ID, HRIP_BLOCK_ID, sizeof(HRIP_BLOCK_ID)))
        {
          break;
        }
        //archive may be trunkated- just stop processing in this case
        if (packedData + fromLE(blockHdr->PackedSize) > ptr + std::min<std::size_t>(size, archiveSize))
        {
          break;
        }
        blocks.push_back(blockHdr);
        offset += fromLE(blockHdr->PackedSize) + (packedData - ptr - offset);
        if (0 != (blockHdr->Flag & HripBlockHeader::LAST_BLOCK))
        {
          break;
        }
      }
      if (blocks.empty() || 0 == (blocks.back()->Flag & HripBlockHeader::LAST_BLOCK))
      {
        continue;
      }
      const TRDos::File::Ptr file = boost::make_shared<HripFile>(source, flatOffset, blocks, ignoreCorrupted);
      flatOffset += file->GetSize();
      builder->AddFile(file);
    }
    builder->SetUsedSize(archiveSize);
    return builder->GetResult();
  }
}

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'H', 'R', 'I', 'P', 0};
  const String VERSION(FromStdString("$Rev$"));
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
      return ParseHripFile(data, parameters);
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
    const ArchivePlugin::Ptr plugin = CreateContainerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
