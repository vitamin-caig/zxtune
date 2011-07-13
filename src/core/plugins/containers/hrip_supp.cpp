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

  class HripDetectionResult : public DetectionResult
  {
  public:
    HripDetectionResult(std::size_t parsedSize, IO::DataContainer::Ptr rawData)
      : ParsedSize(parsedSize)
      , RawData(rawData)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      return ParsedSize;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      const std::size_t size = RawData->Size();

      if (size < sizeof(HripHeader))
      {
        return size;
      }
      const uint8_t* const begin = static_cast<const uint8_t*>(RawData->Data());
      const uint8_t* const end = begin + size;
      return std::search(begin, end, HRIP_ID, ArrayEnd(HRIP_ID)) - begin;
    }
  private:
    const std::size_t ParsedSize;
    const IO::DataContainer::Ptr RawData;
  };

  //files enumeration purposes wrapper
  class Enumerator
  {
  public:
    Enumerator(DataLocation::Ptr location, Plugin::Ptr plugin, const Module::DetectCallback& callback)
      : Location(location)
      , HripPlugin(plugin)
      , Callback(callback)
      , IgnoreCorrupted(CheckIgnoreCorrupted(*Callback.GetPluginsParameters()))
    {
    }

    //main entry
    DetectionResult::Ptr Process()
    {
      const IO::DataContainer::Ptr rawData = Location->GetData();

      uint_t totalFiles = 0, archiveSize = 0;
      while (OK == CheckHrip(rawData->Data(), rawData->Size(), totalFiles, archiveSize))
      {
        if (ParseHrip(rawData->Data(), archiveSize,
            boost::bind(&Enumerator::ProcessFile, this, totalFiles, _1, _2), IgnoreCorrupted) != OK)
        {
          Log::Debug("Core::HRiPSupp", "Failed to parse archive, possible corrupted");
          archiveSize = 0;
        }
        break;
      }
      return boost::make_shared<HripDetectionResult>(archiveSize, rawData);
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
      const String& subPath = TRDos::GetEntryName(header.Name, header.Type);
      //decode
      std::auto_ptr<Dump> decodedData(new Dump());
      if (!DecodeHripFile(headers, IgnoreCorrupted, *decodedData))
      {
        return ERROR;
      }
      const IO::DataContainer::Ptr subData = IO::CreateDataContainer(decodedData);

      if (Log::ProgressCallback* cb = Callback.GetProgress())
      {
        const uint_t progress = 100 * (fileNum + 1) / totalFiles;
        const String path = Location->GetPath()->AsString();
        const String text = path.empty()
          ? Strings::Format(Text::PLUGIN_HRIP_PROGRESS_NOPATH, subPath)
          : Strings::Format(Text::PLUGIN_HRIP_PROGRESS, subPath, path);
        cb->OnProgress(progress, text);
      }

      const DataLocation::Ptr subLocation = CreateNestedLocation(Location, subData, HripPlugin, subPath);
      const Module::NoProgressDetectCallbackAdapter noProgressCallback(Callback);
      Module::Detect(subLocation, noProgressCallback);
      return CONTINUE;
    }
  private:
    const DataLocation::Ptr Location;
    const Plugin::Ptr HripPlugin;
    const Module::DetectCallback& Callback;
    const bool IgnoreCorrupted;
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

  class HRIPPlugin : public ArchivePlugin
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

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      Enumerator cb(input, shared_from_this(), callback);
      return cb.Process();
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
      std::auto_ptr<Dump> dmp(new Dump()); 
      const bool ignoreCorrupted = CheckIgnoreCorrupted(commonParams);
      //ignore corrupted blocks while searching, but try to decode it using proper parameters
      if (OK != ParseHrip(inData->Data(), inData->Size(),
            boost::bind(&FindFileCallback, pathComp, ignoreCorrupted, _1, _2, boost::ref(*dmp)), true))
      {
        Log::Debug("Core::HRiPSupp", "Failed to parse archive, possible corrupted");
        return DataLocation::Ptr();
      }
      if (dmp->empty())
      {
        return DataLocation::Ptr();
      }
      const Plugin::Ptr subPlugin = shared_from_this();
      const IO::DataContainer::Ptr subData = IO::CreateDataContainer(dmp);
      return CreateNestedLocation(location, subData, subPlugin, pathComp); 
    }
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
