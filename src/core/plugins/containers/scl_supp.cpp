/*
Abstract:
  SCL containers support

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
#include <tools.h>

#include <core/error_codes.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <io/container.h>
#include <io/fs_tools.h>

#include <boost/bind.hpp>

#include <numeric>

#include <text/core.h>
#include <text/plugins.h>

#define FILE_TAG 10F03BAF

namespace
{
  using namespace ZXTune;

  const Char SCL_PLUGIN_ID[] = {'S', 'C', 'L', 0};
  const String TEXT_SCL_VERSION(FromStdString("$Rev$"));

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
      return SizeInSectors == ((fromLE(Length) - 1) / BYTES_PER_SECTOR) ?
        fromLE(Length) : BYTES_PER_SECTOR * SizeInSectors;
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

  const std::size_t SCL_MIN_SIZE = sizeof(SCLHeader) + 255 + 4;
  const std::size_t SCL_MODULE_SIZE = sizeof(SCLHeader) - sizeof(SCLEntry) + 
    255 * (sizeof(SCLEntry) + 0xff00) + 4;

  BOOST_STATIC_ASSERT(sizeof(SCLEntry) == 14);
  BOOST_STATIC_ASSERT(sizeof(SCLHeader) == 23);

  typedef std::vector<TRDFileEntry> FileDescriptions;

  uint_t ParseSCLFile(const IO::FastDump& data, FileDescriptions& descrs)
  {
    const uint_t limit = data.Size();
    if (limit < SCL_MIN_SIZE)
    {
      return 0;
    }
    const SCLHeader* const header = safe_ptr_cast<const SCLHeader*>(data.Data());
    if (0 != std::memcmp(header->ID, SINCLAIR_ID, sizeof(SINCLAIR_ID)) ||
        limit < sizeof(*header) + sizeof(header->Blocks) * (header->BlocksCount - 1))
    {
      return 0;
    }
    //TODO: check crc
    const uint_t allSectors = std::accumulate(header->Blocks, header->Blocks + header->BlocksCount, 
      uint_t(0),
      boost::bind(std::plus<uint_t>(), _1, boost::bind<uint_t>(&SCLEntry::SizeInSectors, _2)));
    if (limit < sizeof(*header) + sizeof(header->Blocks) * (header->BlocksCount - 1) + 
      allSectors * BYTES_PER_SECTOR)
    {
      return 0;
    }
    FileDescriptions res;
    res.reserve(header->BlocksCount);
    uint_t offset = safe_ptr_cast<const uint8_t*>(header->Blocks + header->BlocksCount) -
                    safe_ptr_cast<const uint8_t*>(header);
    for (uint_t idx = 0; idx != header->BlocksCount; ++idx)
    {
      const SCLEntry& entry = header->Blocks[idx];
      const TRDFileEntry& newOne = TRDFileEntry(GetTRDosName(entry.Name, entry.Type), offset, entry.Size());
      if (!res.empty() && 
          res.back().IsMergeable(newOne))
      {
        res.back().Merge(newOne);
      }
      else
      {
        res.push_back(newOne);
      }
      offset += entry.SizeInSectors * BYTES_PER_SECTOR;
    }
    descrs.swap(res);
    return offset;
  }

  Error ProcessSCLContainer(const Parameters::Map& commonParams, const DetectParameters& detectParams,
    const MetaContainer& data, ModuleRegion& region)
  {
    FileDescriptions files;
    const uint_t parsedSize = ParseSCLFile(IO::FastDump(*data.Data), files);
    if (!parsedSize)
    {
      return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);
    }

    const PluginsEnumerator& enumerator = PluginsEnumerator::Instance();

    // progress-related
    const bool showMessage = detectParams.Logger != 0;
    Log::MessageData message;
    if (showMessage)
    {
      message.Level = enumerator.CountPluginsInChain(data.PluginsChain, CAP_STOR_MULTITRACK, CAP_STOR_MULTITRACK);
      message.Progress = -1;
    }

    MetaContainer subcontainer;
    subcontainer.PluginsChain = data.PluginsChain;
    subcontainer.PluginsChain.push_back(SCL_PLUGIN_ID);
    ModuleRegion curRegion;
    const uint_t totalCount = files.size();
    uint_t curCount = 0;
    for (FileDescriptions::const_iterator it = files.begin(), lim = files.end(); it != lim; ++it, ++curCount)
    {
      subcontainer.Data = data.Data->GetSubcontainer(it->Offset, it->Size);
      subcontainer.Path = IO::AppendPath(data.Path, it->Name);
      //show progress
      if (showMessage)
      {
        message.Progress = 100 * curCount / totalCount;
        message.Text = (SafeFormatter(data.Path.empty() ? TEXT_PLUGIN_SCL_PROGRESS_NOPATH : TEXT_PLUGIN_SCL_PROGRESS) % it->Name % data.Path).str();
        detectParams.Logger(message);
      }
      if (const Error& err = enumerator.DetectModules(commonParams, detectParams, subcontainer, curRegion))
      {
        return err;
      }
    }
    region.Offset = 0;
    region.Size = parsedSize;
    return Error();
  }

  bool OpenSCLContainer(const Parameters::Map& /*commonParams*/, const MetaContainer& inData, const String& inPath,
    IO::DataContainer::Ptr& outData, String& restPath)
  {
    String restComp;
    const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
    FileDescriptions files;
    if (pathComp.empty() ||
        !ParseSCLFile(IO::FastDump(*inData.Data), files))
    {
      return false;
    }
    const FileDescriptions::const_iterator fileIt = std::find_if(files.begin(), files.end(),
      boost::bind(&TRDFileEntry::Name, _1) == pathComp);
    if (fileIt != files.end())
    {
      outData = inData.Data->GetSubcontainer(fileIt->Offset, fileIt->Size);
      restPath = restComp;
      return true;
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterSCLContainer(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    info.Id = SCL_PLUGIN_ID;
    info.Description = TEXT_SCL_INFO;
    info.Version = TEXT_SCL_VERSION;
    info.Capabilities = CAP_STOR_MULTITRACK | CAP_STOR_PLAIN;
    enumerator.RegisterContainerPlugin(info, OpenSCLContainer, ProcessSCLContainer);
  }
}
