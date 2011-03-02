/*
Abstract:
  HRiP containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "trdos_utils.h"
#include <core/plugins/enumerator.h>
#include <core/plugins/utils.h>
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <parameters.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <formats/packed_decoders.h>
#include <io/container.h>
#include <io/fs_tools.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
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
  inline uint_t CalcCRC(const uint8_t* data, uint_t size)
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

  typedef std::vector<const HripBlockHeader*> HripBlockHeadersList;
  typedef boost::function<CallbackState(uint_t, const HripBlockHeadersList&)> HripCallback;

  //check archive header and calculate files count and total size
  HripResult CheckHrip(const void* data, std::size_t dataSize, uint_t& files, uint_t& archiveSize)
  {
    if (dataSize < sizeof(HripHeader))
    {
      return INVALID;
    }
    const HripHeader* const hripHeader = static_cast<const HripHeader*>(data);
    if (0 != std::memcmp(hripHeader->ID, HRIP_ID, sizeof(HRIP_ID)) ||
       !(0 == hripHeader->Catalogue || 1 == hripHeader->Catalogue) ||
       !hripHeader->ArchiveSectors || !hripHeader->FilesCount)
    {
      return INVALID;
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
    return OK;
  }

  //process all the archive performing callback call on each file
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
      //collect file's blocks
      HripBlockHeadersList blocks;
      for (;;)
      {
        const HripBlockHeader* const blockHdr = safe_ptr_cast<const HripBlockHeader*>(ptr + offset);
        const uint8_t* const packedData = safe_ptr_cast<const uint8_t*>(&blockHdr->PackedCRC) +
          blockHdr->AdditionalSize;
        if (0 != std::memcmp(blockHdr->ID, HRIP_BLOCK_ID, sizeof(HRIP_BLOCK_ID)))
        {
          return CORRUPTED;
        }
        //archive may be trunkated- just stop processing in this case
        if (packedData + fromLE(blockHdr->PackedSize) > ptr + std::min<std::size_t>(size, archiveSize))
        {
          break;
        }
        //check if block CRC is available and check it if required
        if (blockHdr->AdditionalSize > 2 &&
            !ignoreCorrupted &&
            fromLE(blockHdr->PackedCRC) != CalcCRC(packedData, fromLE(blockHdr->PackedSize)))
        {
          return CORRUPTED;
        }
        blocks.push_back(blockHdr);
        offset += fromLE(blockHdr->PackedSize) + (packedData - ptr - offset);
        if (0 != (blockHdr->Flag & blockHdr->LAST_BLOCK))
        {
          break;
        }
      }
      //perform callback call
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

  //append to dst
  bool DecodeHripBlock(const Formats::Packed::Decoder& decoder, const HripBlockHeader* header, Dump& dst)
  {
    const void* const packedData = safe_ptr_cast<const uint8_t*>(&header->PackedCRC) + header->AdditionalSize;
    const uint_t packedSize = fromLE(header->PackedSize);
    if (0 != (header->Flag & header->NO_COMPRESSION))
    {
      //just copy
      dst.resize(fromLE(header->DataSize));
      std::memcpy(&dst[0], packedData, packedSize);
      return true;
    }
    else if (decoder.Check(packedData, packedSize))
    {
      std::size_t usedSize = 0;
      std::auto_ptr<Dump> result = decoder.Decode(packedData, packedSize, usedSize);
      if (result.get() && result->size() == fromLE(header->DataSize))
      {
        result->swap(dst);
        return true;
      }
    }
    Log::Debug("Core::HRiPSupp", "Failed to decode block");
    return false;
  }

  //replace dst
  bool DecodeHripFile(const HripBlockHeadersList& headers, bool ignoreCorrupted, Dump& dst)
  {
    const Formats::Packed::Decoder::Ptr decoder = Formats::Packed::CreateHrust2RawDecoder();
    Dump result;
    for (HripBlockHeadersList::const_iterator it = headers.begin(), lim = headers.end(); it != lim; ++it)
    {
      Dump block;
      const bool isDecoded = DecodeHripBlock(*decoder, *it, block);
      const bool isValid = fromLE((*it)->DataCRC) == CalcCRC(&block[0], block.size());
      if (!(isDecoded && isValid) && !ignoreCorrupted)
      {
        return false;
      }
      const std::size_t sizeBefore = result.size();
      result.resize(sizeBefore + block.size());
      std::copy(block.begin(), block.end(), result.begin() + sizeBefore);
    }
    dst.swap(result);
    return true;
  }

  inline bool CheckIgnoreCorrupted(const Parameters::Accessor& params)
  {
    Parameters::IntType val;
    return params.FindIntValue(Parameters::ZXTune::Core::Plugins::Hrip::IGNORE_CORRUPTED, val) && val != 0;
  }

  //files enumeration purposes wrapper
  class Enumerator
  {
  public:
    Enumerator(Parameters::Accessor::Ptr commonParams, const DetectParameters& detectParams,
      const MetaContainer& data)
      : Params(commonParams)
      , IgnoreCorrupted(CheckIgnoreCorrupted(*Params))
      , DetectParams(detectParams)
      , Container(data.Data)
      , Path(data.Path)
      , LogLevel(-1)
      , SubMetacontainer(data)
    {
    }

    //main entry
    bool Process(ModuleRegion& region)
    {
      uint_t totalFiles = 0, archiveSize = 0;

      if (OK != CheckHrip(Container->Data(), Container->Size(), totalFiles, archiveSize))
      {
        return false;
      }
      if (ParseHrip(Container->Data(), archiveSize,
            boost::bind(&Enumerator::ProcessFile, this, totalFiles, _1, _2), IgnoreCorrupted) != OK)
      {
        Log::Debug("Core::HRiPSupp", "Failed to parse archive, possible corrupted");
        return false;
      }
      region.Offset = 0;
      region.Size = archiveSize;
      return true;
    }

  private:
    CallbackState ProcessFile(uint_t totalFiles, uint_t fileNum, const HripBlockHeadersList& headers)
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
      const String& filename = TRDos::GetEntryName(header.Name, header.Type);
      //decode
      {
        Dump tmp;
        if (!DecodeHripFile(headers, IgnoreCorrupted, tmp))
        {
          return ERROR;
        }
        SubMetacontainer.Data = IO::CreateDataContainer(tmp);
        SubMetacontainer.Path = IO::AppendPath(Path, filename);
      }

      {
        Log::MessageData message;
        message.Level = GetLogLevel();
        message.Progress = 100 * (fileNum + 1) / totalFiles;
        message.Text = (SafeFormatter(Path.empty() ? Text::PLUGIN_HRIP_PROGRESS_NOPATH : Text::PLUGIN_HRIP_PROGRESS) % filename % Path).str();
        DetectParams.ReportMessage(message);
      }

      ModuleRegion curRegion;
      PluginsEnumerator::Instance().DetectModules(Params, DetectParams, SubMetacontainer, curRegion);
      return CONTINUE;
    }
  private:
    //for optimization purposes
    uint_t GetLogLevel() const
    {
      if (-1 == LogLevel)
      {
        LogLevel = static_cast<int_t>(
          SubMetacontainer.Plugins->CalculateContainersNesting() - 1);
      }
      return LogLevel;
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const bool IgnoreCorrupted;
    const DetectParameters& DetectParams;
    const IO::DataContainer::Ptr Container;
    const String Path;
    mutable int_t LogLevel;
    MetaContainer SubMetacontainer;
  };

  CallbackState FindFileCallback(const String& filename, bool ignoreCorrupted, uint_t /*fileNum*/,
    const HripBlockHeadersList& headers, Dump& dst)
  {
    if (!headers.empty() &&
        filename == TRDos::GetEntryName(headers.front()->Name, headers.front()->Type))//found file
    {
      return DecodeHripFile(headers, ignoreCorrupted, dst) ? EXIT : ERROR;
    }
    return CONTINUE;
  }

  class HRIPPlugin : public ContainerPlugin
                   , public boost::enable_shared_from_this<HRIPPlugin>
  {
  public:
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

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      uint_t filesCount = 0;
      uint_t archiveSize = 0;
      return INVALID != CheckHrip(inputData.Data(), inputData.Size(), filesCount, archiveSize) &&
             filesCount != 0;
    }

    virtual bool Process(Parameters::Accessor::Ptr params, const DetectParameters& detectParams,
      const MetaContainer& data, ModuleRegion& region) const
    {
      MetaContainer nested(data);
      nested.Plugins->Add(shared_from_this());
      Enumerator cb(params, detectParams, nested);
      return cb.Process(region);
    }

    virtual IO::DataContainer::Ptr Open(const Parameters::Accessor& commonParams,
      const MetaContainer& inData, const String& inPath,
      String& restPath) const
    {
      String restComp;
      const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
      if (pathComp.empty())
      {
        //nothing to open
        return IO::DataContainer::Ptr();
      }
      Dump dmp;
      const bool ignoreCorrupted = CheckIgnoreCorrupted(commonParams);
      const IO::DataContainer& container = *inData.Data;
      //ignore corrupted blocks while searching, but try to decode it using proper parameters
      if (OK != ParseHrip(container.Data(), container.Size(),
            boost::bind(&FindFileCallback, pathComp, ignoreCorrupted, _1, _2, boost::ref(dmp)), true))
      {
        Log::Debug("Core::HRiPSupp", "Failed to parse archive, possible corrupted");
        return IO::DataContainer::Ptr();
      }
      if (dmp.empty())
      {
        return IO::DataContainer::Ptr();
      }
      restPath = restComp;
      return IO::CreateDataContainer(dmp);
    }
  };
}

namespace ZXTune
{
  void RegisterHRIPContainer(PluginsEnumerator& enumerator)
  {
    const ContainerPlugin::Ptr plugin(new HRIPPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
