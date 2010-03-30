/*
Abstract:
  HRiP containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"
#include "../utils.h"

#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <parameters.h>
#include <tools.h>

#include <core/error_codes.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <io/container.h>
#include <io/fs_tools.h>

#include <boost/bind.hpp>

#include <text/core.h>
#include <text/plugins.h>

#define FILE_TAG 942A64CC

namespace
{
  using namespace ZXTune;

  const Char HRIP_PLUGIN_ID[] = {'H', 'R', 'I', 'P', 0};

  const String TEXT_HRIP_VERSION(FromStdString("$Rev$"));
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
  //BOOST_STATIC_ASSERT(sizeof(HripHeader::ID) == sizeof(HRIP_ID));
  BOOST_STATIC_ASSERT(sizeof(HripBlockHeader) == 29);
  //BOOST_STATIC_ASSERT(sizeof(HripBlockHeader::ID) == sizeof(HRIP_BLOCK_ID));

  inline uint_t CalcCRC(const uint8_t* data, uint_t size)
  {
    uint_t result(0);
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

  enum HripResult
  {
    OK,
    INVALID,
    CORRUPTED
  };

  enum CallbackState
  {
    CONTINUE,
    EXIT,
    ERROR,
  };

  typedef boost::function<CallbackState(uint_t, const std::vector<const HripBlockHeader*>&)> HripCallback;

  HripResult CheckHrip(const void* data, std::size_t dataSize, uint_t& files, uint_t& archiveSize)
  {
    if (dataSize < sizeof(HripHeader))
    {
      return INVALID;
    }
    const HripHeader* const hripHeader = static_cast<const HripHeader*>(data);
    if (0 != std::memcmp(hripHeader->ID, HRIP_ID, sizeof(HRIP_ID)) ||
      !(0 == hripHeader->Catalogue || 1 == hripHeader->Catalogue))
    {
      return INVALID;
    }
    files = hripHeader->FilesCount;
    archiveSize = 256 * (fromLE(hripHeader->ArchiveSectors) - 1) + hripHeader->UsedInLastSector;
    return OK;
  }

  HripResult ParseHrip(const void* data, std::size_t size, const HripCallback& callback, bool ignoreCorrupted)
  {
    uint_t files = 0, archiveSize = 0;
    const HripResult checkRes = CheckHrip(data, size, files, archiveSize);
    if (checkRes != OK)
    {
      return checkRes;
    }
    const uint8_t* const ptr = static_cast<const uint8_t*>(data);
    std::size_t offset = sizeof(HripHeader);
    for (uint_t fileNum = 0; fileNum < files; ++fileNum)
    {
      std::vector<const HripBlockHeader*> blocks;
      for (;;)//for blocks of file
      {
        const HripBlockHeader* const blockHdr = safe_ptr_cast<const HripBlockHeader*>(ptr + offset);
        const uint8_t* const packedData = safe_ptr_cast<const uint8_t*>(&blockHdr->PackedCRC) +
          blockHdr->AdditionalSize;
        if (0 != std::memcmp(blockHdr->ID, HRIP_BLOCK_ID, sizeof(HRIP_BLOCK_ID)))
        {
          return CORRUPTED;
        }
        if (packedData + fromLE(blockHdr->PackedSize) > ptr + std::min(size, archiveSize))//out of data
        {
          break;
        }
        if (blockHdr->AdditionalSize > 2 && //here's CRC
            fromLE(blockHdr->PackedCRC) != CalcCRC(packedData, fromLE(blockHdr->PackedSize)) &&
            !ignoreCorrupted)
        {
          return CORRUPTED;
        }
        blocks.push_back(blockHdr);
        offset += fromLE(blockHdr->PackedSize) + (packedData - ptr - offset);
        if (blockHdr->Flag & blockHdr->LAST_BLOCK)
        {
          break;
        }
      }
      switch (callback(fileNum, blocks))
      {
      case ERROR:
        return CORRUPTED;
      case EXIT:
        return OK;
      default:
        break;
      }
    }
    return OK;
  }

  //append
  bool DecodeHripBlock(const HripBlockHeader* header, Dump& dst)
  {
    const void* const packedData = safe_ptr_cast<const uint8_t*>(&header->PackedCRC) + header->AdditionalSize;
    const std::size_t sizeBefore = dst.size();
    if (header->Flag & header->NO_COMPRESSION)
    {
      dst.resize(sizeBefore + fromLE(header->DataSize));
      std::memcpy(&dst[sizeBefore], packedData, fromLE(header->PackedSize));
      return true;
    }
    dst.reserve(sizeBefore + fromLE(header->DataSize));
    return ::DecodeHrust2x(static_cast<const Hrust2xHeader*>(packedData), fromLE(header->PackedSize), dst) &&
      dst.size() == sizeBefore + fromLE(header->DataSize);
  }

  inline bool CheckIgnoreCorrupted(const Parameters::Map& params)
  {
    Parameters::IntType val;
    return Parameters::FindByName(params, Parameters::ZXTune::Core::Plugins::Hrip::IGNORE_CORRUPTED, val) && val != 0;
  }

  class Enumerator
  {
  public:
    Enumerator(const Parameters::Map& commonParams, const DetectParameters& detectParams,
      const MetaContainer& data)
      : Params(commonParams)
      , IgnoreCorrupted(CheckIgnoreCorrupted(commonParams))
      , DetectParams(detectParams)
      , Container(data.Data)
      , Path(data.Path)
      , LogLevel(PluginsEnumerator::Instance().CountPluginsInChain(
         data.PluginsChain, CAP_STOR_MULTITRACK, CAP_STOR_MULTITRACK))
      , SubMetacontainer(data)
    {
      SubMetacontainer.PluginsChain.push_back(HRIP_PLUGIN_ID);
    }

    Error Process(ModuleRegion& region)
    {
      try
      {
        uint_t totalFiles = 0, archiveSize = 0;

        if (CheckHrip(Container->Data(), Container->Size(), totalFiles, archiveSize) != OK ||
            ParseHrip(Container->Data(), archiveSize,
              boost::bind(&Enumerator::ProcessFile, this, totalFiles, _1, _2), IgnoreCorrupted) != OK)
        {
          return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);
        }
        region.Offset = 0;
        region.Size = archiveSize;
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }

  private:
    CallbackState ProcessFile(uint_t totalFiles, uint_t fileNum, const std::vector<const HripBlockHeader*>& headers)
    {
      if (headers.empty())
      {
        return ERROR;
      }
      const HripBlockHeader& header = *headers.front();
      if (header.AdditionalSize < 15)
      {
        return ERROR;
      }
      const String filename(GetTRDosName(header.Name, header.Type));
      //decode
      {
        Dump tmp;
        if (headers.end() != std::find_if(headers.begin(), headers.end(),
              !boost::bind(&DecodeHripBlock, _1, boost::ref(tmp))) && !IgnoreCorrupted)
        {
          return ERROR;
        }
        SubMetacontainer.Data = IO::CreateDataContainer(tmp);
        SubMetacontainer.Path = IO::AppendPath(Path, filename);
      }
      if (DetectParams.Logger)
      {
        Log::MessageData message;
        message.Level = LogLevel;
        message.Progress = 100 * (fileNum + 1) / totalFiles;
        message.Text = (SafeFormatter(Path.empty() ? TEXT_PLUGIN_HRIP_PROGRESS_NOPATH : TEXT_PLUGIN_HRIP_PROGRESS) % filename % Path).str();
        DetectParams.Logger(message);
      }

      ModuleRegion curRegion;
      ThrowIfError(PluginsEnumerator::Instance().DetectModules(Params, DetectParams, SubMetacontainer, curRegion));
      return CONTINUE;
    }
  private:
    const Parameters::Map& Params;
    const bool IgnoreCorrupted;
    const DetectParameters& DetectParams;
    const IO::DataContainer::Ptr Container;
    const String Path;
    const uint_t LogLevel;
    MetaContainer SubMetacontainer;
  };

  Error ProcessHRIPContainer(const Parameters::Map& commonParams, const DetectParameters& detectParams,
    const MetaContainer& data, ModuleRegion& region)
  {
    Enumerator cb(commonParams, detectParams, data);
    return cb.Process(region);
  }

  CallbackState FindFileCallback(const String& filename, bool ignoreCorrupted, uint_t /*fileNum*/,
    const std::vector<const HripBlockHeader*>& headers, Dump& dst)
  {
    if (!headers.empty() &&
        filename == GetTRDosName(headers.front()->Name, headers.front()->Type))//found file
    {
      Dump tmp;
      if (headers.end() == std::find_if(headers.begin(), headers.end(),
          !boost::bind(&DecodeHripBlock, _1, boost::ref(tmp))) && !ignoreCorrupted)
      {
        dst.swap(tmp);
        return EXIT;
      }
      return ERROR;
    }
    return CONTINUE;
  }

  bool OpenHRIPContainer(const Parameters::Map& commonParams, const MetaContainer& inData, const String& inPath,
    IO::DataContainer::Ptr& outData, String& restPath)
  {
    String restComp;
    const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
    Dump dmp;
    const bool ignoreCorrupted = CheckIgnoreCorrupted(commonParams);
    const IO::DataContainer& container = *inData.Data;
    if (pathComp.empty() ||
        OK != ParseHrip(container.Data(), container.Size(),
          boost::bind(&FindFileCallback, pathComp, ignoreCorrupted, _1, _2, boost::ref(dmp)), ignoreCorrupted) ||
        dmp.empty())
    {
      return false;
    }
    outData = IO::CreateDataContainer(dmp);
    restPath = restComp;
    return true;
  }
}

namespace ZXTune
{
  void RegisterHRIPContainer(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    info.Id = HRIP_PLUGIN_ID;
    info.Description = TEXT_HRIP_INFO;
    info.Version = TEXT_HRIP_VERSION;
    info.Capabilities = CAP_STOR_MULTITRACK;
    enumerator.RegisterContainerPlugin(info, OpenHRIPContainer, ProcessHRIPContainer);
  }
}
