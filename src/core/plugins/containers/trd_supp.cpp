/*
Abstract:
  TRD containers support

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

#include <text/core.h>
#include <text/plugins.h>

#define FILE_TAG A1239034

namespace
{
  using namespace ZXTune;

  const Char TRD_PLUGIN_ID[] = {'T', 'R', 'D', 0};
  
  const String TEXT_TRD_VERSION(FromStdString("$Rev$"));

  const std::size_t TRD_MODULE_SIZE = 655360;
  const uint_t BYTES_PER_SECTOR = 256;
  const uint_t SECTORS_IN_TRACK = 16;
  const uint_t MAX_FILES_COUNT = 128;
  const uint_t SERVICE_SECTOR_NUM = 8;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif

  enum
  {
    NOENTRY = 0,
    DELETED = 1
  };
  PACK_PRE struct CatEntry
  {
    char Name[8];
    char Type[3];
    uint16_t Length;
    uint8_t SizeInSectors;
    uint8_t Sector;
    uint8_t Track;

    uint_t Offset() const
    {
      return BYTES_PER_SECTOR * (SECTORS_IN_TRACK * Track + Sector);
    }

    uint_t Size() const
    {
      return SizeInSectors == ((fromLE(Length) - 1) / BYTES_PER_SECTOR) ?
        fromLE(Length) : BYTES_PER_SECTOR * SizeInSectors;
    }
  } PACK_POST;

  enum
  {
    TRDOS_ID = 0x10,

    DS_DD = 0x16,
    DS_SD = 0x17,
    SS_DD = 0x18,
    SS_SD = 0x19
  };
  PACK_PRE struct ServiceSector
  {
    uint8_t Zero;
    uint8_t Reserved1[224];
    uint8_t FreeSpaceSect;
    uint8_t FreeSpaceTrack;
    uint8_t Type;
    uint8_t Files;
    uint16_t FreeSectors;
    uint8_t ID;//0x10
    uint8_t Reserved2[12];
    uint8_t DeletedFiles;
    uint8_t Title[8];
    uint8_t Reserved3[3];
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CatEntry) == 16);
  BOOST_STATIC_ASSERT(sizeof(ServiceSector) == 256);

  typedef std::vector<TRDFileEntry> FileDescriptions;

  bool ParseTRDFile(const IO::FastDump& data, FileDescriptions& descrs)
  {
    if (data.Size() < TRD_MODULE_SIZE)
    {
      return false;
    }
    const ServiceSector* const sector = safe_ptr_cast<const ServiceSector*>(&data[SERVICE_SECTOR_NUM * BYTES_PER_SECTOR]);
    if (sector->ID != TRDOS_ID || sector->Type != DS_DD || 0 != sector->Zero)
    {
      return false;
    }

    FileDescriptions res;
    res.reserve(MAX_FILES_COUNT);

    const CatEntry* catEntry = safe_ptr_cast<const CatEntry*>(data.Data());
    for (uint_t idx = 0; idx != MAX_FILES_COUNT && NOENTRY != catEntry->Name[0]; ++idx, ++catEntry)
    {
      if (DELETED != catEntry->Name[0] && catEntry->SizeInSectors)
      {
        if (catEntry->Offset() + catEntry->Size() > TRD_MODULE_SIZE)
        {
          return false;
        }
        const TRDFileEntry& newOne =
          TRDFileEntry(GetTRDosName(catEntry->Name, catEntry->Type), catEntry->Offset(), catEntry->Size());
        if (!res.empty() && res.back().IsMergeable(newOne))
        {
          res.back().Merge(newOne);
        }
        else
        {
          res.push_back(newOne);
        }
      }
    }
    descrs.swap(res);
    return true;
  }
  
  
  Error ProcessTRDContainer(const Parameters::Map& commonParams, const DetectParameters& detectParams,
    const MetaContainer& data, ModuleRegion& region)
  {
    //do not search if there's already TRD plugin (cannot be nested...)
    if (!data.PluginsChain.empty() &&
        data.PluginsChain.end() != std::find(data.PluginsChain.begin(), data.PluginsChain.end(), TRD_PLUGIN_ID))
    {
      return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);
    }
    FileDescriptions files;
    if (!ParseTRDFile(IO::FastDump(*data.Data), files))
    {
      return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);
    }

    // progress-related
    const bool showMessage = detectParams.Logger != 0;
    Log::MessageData message;
    if (showMessage)
    {
      message.Level = data.PluginsChain.size();
      message.Progress = -1;
    }

    const PluginsEnumerator& enumerator = PluginsEnumerator::Instance();
    MetaContainer subcontainer;
    subcontainer.PluginsChain = data.PluginsChain;
    subcontainer.PluginsChain.push_back(TRD_PLUGIN_ID);
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
        message.Text = (SafeFormatter(data.Path.empty() ? TEXT_PLUGIN_TRD_PROGRESS_NOPATH : TEXT_PLUGIN_TRD_PROGRESS) % it->Name % data.Path).str();
        detectParams.Logger(message);
      }
      if (const Error& err = enumerator.DetectModules(commonParams, detectParams, subcontainer, curRegion))
      {
        return err;
      }
    }
    region.Offset = 0;
    region.Size = TRD_MODULE_SIZE;
    return Error();
  }
  
  bool OpenTRDContainer(const Parameters::Map& /*commonParams*/, const MetaContainer& inData, const String& inPath,
    IO::DataContainer::Ptr& outData, String& restPath)
  {
    String restComp;
    const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
    FileDescriptions files;
    if (pathComp.empty() ||
        !ParseTRDFile(IO::FastDump(*inData.Data), files))
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
  void RegisterTRDContainer(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    info.Id = TRD_PLUGIN_ID;
    info.Description = TEXT_TRD_INFO;
    info.Version = TEXT_TRD_VERSION;
    info.Capabilities = CAP_STOR_MULTITRACK | CAP_STOR_PLAIN;
    enumerator.RegisterContainerPlugin(info, OpenTRDContainer, ProcessTRDContainer);
  }
}
