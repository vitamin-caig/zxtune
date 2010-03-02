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

  class HripCallback
  {
  public:
    virtual ~HripCallback() {}

    // true if ignore block
    virtual bool IgnoreCorruptedBlock() const = 0;

    enum CallbackState
    {
      CONTINUE,
      EXIT,
      ERROR,
    };

    virtual CallbackState ProcessFile(const std::vector<const HripBlockHeader*>& blocks) = 0;
  };

  HripResult ParseHrip(const void* data, std::size_t size, HripCallback& callback)
  {
    if (size < sizeof(HripHeader))
    {
      return INVALID;
    }
    const HripHeader* const hripHeader(static_cast<const HripHeader*>(data));
    if (0 != std::memcmp(hripHeader->ID, HRIP_ID, sizeof(HRIP_ID)) &&
      !(0 == hripHeader->Catalogue || 1 == hripHeader->Catalogue))
    {
      return INVALID;
    }
    const std::size_t archiveSize(256 * (fromLE(hripHeader->ArchiveSectors) - 1) + hripHeader->UsedInLastSector);
    const uint8_t* const ptr(static_cast<const uint8_t*>(data));
    std::size_t offset(sizeof(*hripHeader));
    for (uint_t fileNum = 0; fileNum < hripHeader->FilesCount; ++fileNum)
    {
      std::vector<const HripBlockHeader*> blocks;
      for (;;)//for blocks of file
      {
        const HripBlockHeader* const blockHdr(safe_ptr_cast<const HripBlockHeader*>(ptr + offset));
        const uint8_t* const packedData(safe_ptr_cast<const uint8_t*>(&blockHdr->PackedCRC) +
          blockHdr->AdditionalSize);
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
            !callback.IgnoreCorruptedBlock())
        {
          return CORRUPTED;
        }
        blocks.push_back(blockHdr);
        offset += fromLE(blockHdr->PackedSize + (packedData - ptr - offset));
        if (blockHdr->Flag & blockHdr->LAST_BLOCK)
        {
          break;
        }
      }
      const HripCallback::CallbackState state(callback.ProcessFile(blocks));
      if (HripCallback::ERROR == state)
      {
        return CORRUPTED;
      }
      else if (HripCallback::EXIT == state)
      {
        return OK;
      }
    }
    return OK;
  }

  //append
  bool DecodeHripBlock(const HripBlockHeader* header, Dump& dst)
  {
    const void* const packedData(safe_ptr_cast<const uint8_t*>(&header->PackedCRC) + header->AdditionalSize);
    const std::size_t sizeBefore(dst.size());
    if (header->Flag & header->NO_COMPRESSION)
    {
      dst.resize(sizeBefore + fromLE(header->DataSize));
      std::memcpy(&dst[sizeBefore], packedData, fromLE(header->PackedSize));
      return true;
    }
    dst.reserve(sizeBefore + fromLE(header->DataSize));
    return ::DecodeHrust2x(static_cast<const Hrust2xHeader*>(packedData), header->PackedSize, dst) &&
      dst.size() == sizeBefore + fromLE(header->DataSize);
  }

  class BasicCallback : public HripCallback
  {
  public:
    explicit BasicCallback(const Parameters::Map& params)
      : IgnoreCorrupted(false)
    {
      Parameters::IntType val;
      IgnoreCorrupted = Parameters::FindByName(params, Parameters::ZXTune::Core::Plugins::Hrip::IGNORE_CORRUPTED, val) && val != 0;
    }

    virtual bool IgnoreCorruptedBlock() const
    {
      return IgnoreCorrupted;
    }
  protected:
    bool IgnoreCorrupted;
  };

  /*
  class EnumerateCallback : public HripCallback
  {
  public:
    EnumerateCallback()
    {
    }

    virtual CallbackState OnFile(const std::vector<const HripBlockHeader*>& headers)
    {
      const HripBlockHeader* const header(headers.front());
      Files.push_back(HripFile(IO::GetFileName(header->Filename), header->Filesize));
      return CONTINUE;
    }

    void GetFiles(HripFiles& files)
    {
      files.swap(Files);
    }
  private:
    HripFiles Files;
  };
  */

  class OpenFileCallback : public BasicCallback
  {
  public:
    OpenFileCallback(const Parameters::Map& params, const String& filename, Dump& dst)
      : BasicCallback(params)
      , Filename(filename), Dst(dst)
    {
    }

    virtual CallbackState ProcessFile(const std::vector<const HripBlockHeader*>& headers)
    {
      if (!headers.empty() &&
          Filename == GetTRDosName(headers.front()->Name, headers.front()->Type))//found file
      {
        Dump tmp;
        if (headers.end() == std::find_if(headers.begin(), headers.end(),
          !boost::bind(&DecodeHripBlock, _1, boost::ref(tmp))) && !IgnoreCorrupted)
        {
          Dst.swap(tmp);
          return EXIT;
        }
        return ERROR;
      }
      return CONTINUE;
    }
  private:
    const String Filename;
    Dump& Dst;
  };
}

namespace ZXTune
{
  Error ProcessHRIPContainer(const Parameters::Map& commonParams, const DetectParameters& detectParams,
    const MetaContainer& data, ModuleRegion& region)
  {
    return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);
  }

  bool OpenHRIPContainer(const Parameters::Map& commonParams, const MetaContainer& inData, const String& inPath,
    IO::DataContainer::Ptr& outData, String& restPath)
  {
    String restComp;
    const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
    Dump dmp;
    OpenFileCallback cb(commonParams, pathComp, dmp);
    const IO::DataContainer& container(*inData.Data);
    if (pathComp.empty() ||
        ParseHrip(container.Data(), container.Size(), cb) != OK)
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
