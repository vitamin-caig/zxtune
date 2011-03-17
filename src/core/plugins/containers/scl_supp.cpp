/*
Abstract:
  SCL containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "trdos_process.h"
#include "core/src/callback.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/src/core.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <io/container.h>
#include <io/fs_tools.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 10F03BAF

namespace
{
  using namespace ZXTune;

  const Char SCL_PLUGIN_ID[] = {'S', 'C', 'L', 0};
  const String SCL_PLUGIN_VERSION(FromStdString("$Rev$"));

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

  bool CheckSCLFile(const IO::FastDump& data)
  {
    const uint_t limit = data.Size();
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
    const uint_t descriptionsSize = sizeof(*header) + sizeof(header->Blocks) * (header->BlocksCount - 1);
    if (descriptionsSize > limit)
    {
      return false;
    }
    const uint_t dataSize = std::accumulate(header->Blocks, header->Blocks + header->BlocksCount, 0, &SumDataSize);
    if (descriptionsSize + dataSize + sizeof(uint32_t) > limit)
    {
      return false;
    }
    const uint_t checksumOffset = descriptionsSize + dataSize;
    const uint32_t storedChecksum = fromLE(*safe_ptr_cast<const uint32_t*>(data.Data() + checksumOffset));
    const uint32_t checksum = std::accumulate(data.Data(), data.Data() + checksumOffset, uint32_t(0));
    return storedChecksum == checksum;
  }

  //fill descriptors array and return actual container size
  TRDos::FilesSet::Ptr ParseSCLFile(const IO::FastDump& data, std::size_t& parsedSize)
  {
    assert(CheckSCLFile(data));
    const SCLHeader* const header = safe_ptr_cast<const SCLHeader*>(data.Data());

    TRDos::FilesSet::Ptr res = TRDos::FilesSet::Create();
    //res.reserve(header->BlocksCount);
    std::size_t offset = safe_ptr_cast<const uint8_t*>(header->Blocks + header->BlocksCount) -
                    safe_ptr_cast<const uint8_t*>(header);
    for (uint_t idx = 0; idx != header->BlocksCount; ++idx)
    {
      const SCLEntry& entry = header->Blocks[idx];
      const std::size_t nextOffset = offset + entry.Size();
      const String entryName = TRDos::GetEntryName(entry.Name, entry.Type);
      const TRDos::FileEntry& newOne = TRDos::FileEntry(entryName, offset, entry.Size());
      res->AddEntry(newOne);
      offset = nextOffset;
    }
    parsedSize = offset;
    return res;
  }

  class SCLPlugin : public ContainerPlugin
                  , public boost::enable_shared_from_this<SCLPlugin>
  {
  public:
    virtual String Id() const
    {
      return SCL_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::SCL_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return SCL_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MULTITRACK | CAP_STOR_PLAIN;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const IO::FastDump dump(inputData);
      return CheckSCLFile(dump);
    }

    virtual std::size_t Process(DataLocation::Ptr location, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr data = location->GetData();
      const IO::FastDump dump(*data);
      if (!CheckSCLFile(dump))
      {
        return 0;
      }
      std::size_t parsedSize = 0;
      const TRDos::FilesSet::Ptr files = ParseSCLFile(dump, parsedSize);
      if (files->GetEntriesCount())
      {
        ProcessEntries(location, callback, shared_from_this(), *files);
      }
      return parsedSize;
    }

    IO::DataContainer::Ptr Open(const Parameters::Accessor& /*commonParams*/,
      const MetaContainer& inData, const String& inPath, String& restPath) const
    {
      String restComp;
      const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
      if (pathComp.empty())
      {
        return IO::DataContainer::Ptr();
      }
      const IO::FastDump dump(*inData.Data);
      std::size_t parsedSize = 0;
      const TRDos::FilesSet::Ptr files = ParseSCLFile(dump, parsedSize);
      const TRDos::FileEntry* const entryToOpen = files.get()
        ? files->FindEntry(pathComp)
        : 0;
      if (!entryToOpen)
      {
        return IO::DataContainer::Ptr();
      }
      restPath = restComp;
      return inData.Data->GetSubcontainer(entryToOpen->Offset, entryToOpen->Size);
    }
  };
}

namespace ZXTune
{
  void RegisterSCLContainer(PluginsRegistrator& registrator)
  {
    const ContainerPlugin::Ptr plugin(new SCLPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
