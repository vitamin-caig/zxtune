/*
Abstract:
  HRiP containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "trdos_process.h"
#include <core/src/callback.h>
#include <core/src/core.h>
#include <core/plugins/registrator.h>
#include <core/plugins/utils.h>
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <parameters.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <formats/packed_decoders.h>
#include <io/container.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 942A64CC

namespace
{
  using namespace ZXTune;

  const Char HRIP_PLUGIN_ID[] = {'H', 'R', 'I', 'P', 0};
  const String HRIP_PLUGIN_VERSION(FromStdString("$Rev$"));

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

    explicit DataSource(IO::DataContainer::Ptr data)
      : Decoder(Formats::Packed::CreateHrust2RawDecoder())
      , Data(data)
    {
    }

    Dump DecodeBlock(const HripBlockHeader* block) const
    {
      const uint8_t* const packedData = safe_ptr_cast<const uint8_t*>(&block->PackedCRC) + block->AdditionalSize;
      const std::size_t packedSize = fromLE(block->PackedSize);
      const std::size_t unpackedSize = fromLE(block->DataSize);
      Dump res;
      if (0 != (block->Flag & HripBlockHeader::NO_COMPRESSION))
      {
        //just copy
        res.assign(packedData, packedData + packedSize);
      }
      else if (Decoder->Check(packedData, packedSize))
      {
        std::size_t usedSize = 0;
        std::auto_ptr<Dump> result = Decoder->Decode(packedData, packedSize, usedSize);
        if (result.get())
        {
          result->swap(res);
        }
      }
      if (unpackedSize != res.size())
      {
        res.resize(unpackedSize);
      }
      return res;
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
    const IO::DataContainer::Ptr Data;
  };

  typedef std::vector<const HripBlockHeader*> HripBlockHeadersList;

  class HripFile : public TRDos::File
  {
  public:
    HripFile(DataSource::Ptr data, std::size_t offset, const HripBlockHeadersList& blocks)
      : Data(data)
      , Offset(offset)
      , Blocks(blocks)
      , FirstBlock(*Blocks.front())
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

    virtual IO::DataContainer::Ptr GetData() const
    {
      return Result;
    }

    bool Decode(bool ignoreCorrupted)
    {
      std::auto_ptr<Dump> result(new Dump(GetSize()));
      Dump::iterator dst = result->begin();
      for (HripBlockHeadersList::const_iterator it = Blocks.begin(), lim = Blocks.end(); it != lim; ++it)
      {
        const HripBlockHeader* const block = *it;
        const Dump& data = Data->DecodeBlock(block);
        if (!ignoreCorrupted)
        {
          const uint8_t* const packedData = safe_ptr_cast<const uint8_t*>(&block->PackedCRC) + block->AdditionalSize;
          //check if block CRC is available and check it if required
          if (block->AdditionalSize >= 2 &&
              fromLE(block->PackedCRC) != CalcCRC(packedData, fromLE(block->PackedSize)))
          {
            return false;
          }
          if (block->AdditionalSize >= 4 &&
              fromLE(block->DataCRC) != CalcCRC(&data[0], data.size()))
          {
            return false;
          }
        }
        dst = std::copy(data.begin(), data.end(), dst);
      }
      Result = IO::CreateDataContainer(result);
      return true;
    }
  private:
    const DataSource::Ptr Data;
    const std::size_t Offset;
    const HripBlockHeadersList Blocks;
    const HripBlockHeader& FirstBlock;
    mutable IO::DataContainer::Ptr Result;
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


  TRDos::Catalogue::Ptr ParseHripFile(IO::DataContainer::Ptr data, const Parameters::Accessor& params)
  {
    uint_t files = 0;
    std::size_t archiveSize = 0;
    const uint8_t* const ptr = static_cast<const uint8_t*>(data->Data());
    const std::size_t size = data->Size();
    if (!CheckHrip(ptr, size, files, archiveSize))
    {
      return TRDos::Catalogue::Ptr();
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
      std::auto_ptr<HripFile> file(new HripFile(source, flatOffset, blocks));
      flatOffset += file->GetSize();
      if (file->Decode(ignoreCorrupted))
      {
        builder->AddFile(TRDos::File::Ptr(file.release()));
      }
    }
    builder->SetUsedSize(archiveSize);
    return builder->GetResult();
  }

  const std::string HRIP_FORMAT(
    "'H'R'i" //uint8_t ID[3];//'HRi'
    "01-ff"  //uint8_t FilesCount;
    "?"      //uint8_t UsedInLastSector;
    "??"     //uint16_t ArchiveSectors;
    "%0000000x"//uint8_t Catalogue;
  );

  class HRIPPlugin : public ArchivePlugin
                   , public boost::enable_shared_from_this<HRIPPlugin>
  {
  public:
    HRIPPlugin()
      : Format(DataFormat::Create(HRIP_FORMAT))
    {
    }

    virtual String Id() const
    {
      return HRIP_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::HRIP_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return HRIP_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MULTITRACK;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr rawData = input->GetData();
      if (const TRDos::Catalogue::Ptr files = ParseHripFile(rawData, *callback.GetPluginsParameters()))
      {
        if (files->GetFilesCount())
        {
          TRDos::ProcessEntries(input, callback, shared_from_this(), *files);
          return DetectionResult::CreateMatched(files->GetUsedSize());
        }
      }
      return DetectionResult::CreateUnmatched(Format, rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& commonParams, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const String& pathComp = inPath.GetFirstComponent();
      if (pathComp.empty())
      {
        //nothing to open
        return DataLocation::Ptr();
      }
      const IO::DataContainer::Ptr inData = location->GetData();
      if (const TRDos::Catalogue::Ptr files = ParseHripFile(inData, commonParams))
      {
        if (const TRDos::File::Ptr fileToOpen = files->FindFile(pathComp))
        {
          const Plugin::Ptr subPlugin = shared_from_this();
          const IO::DataContainer::Ptr subData = fileToOpen->GetData();
          return CreateNestedLocation(location, subData, subPlugin, pathComp); 
        }
      }
      return DataLocation::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterHRIPContainer(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new HRIPPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
