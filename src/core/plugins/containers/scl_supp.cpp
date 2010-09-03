/*
Abstract:
  SCL containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/plugins/enumerator.h>
#include <core/plugins/utils.h>
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
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

  typedef std::vector<TRDFileEntry> FileDescriptions;

  bool CheckSCLFile(const IO::FastDump& data)
  {
    const uint_t limit = data.Size();
    if (limit < SCL_MIN_SIZE)
    {
      return false;
    }
    const SCLHeader* const header = safe_ptr_cast<const SCLHeader*>(data.Data());
    if (0 != std::memcmp(header->ID, SINCLAIR_ID, sizeof(SINCLAIR_ID)) ||
        0 == header->BlocksCount ||
        //minimal size according to blocks count
        limit < sizeof(*header) - sizeof(header->Blocks) + (BYTES_PER_SECTOR + sizeof(header->Blocks) * header->BlocksCount)
       )
    {
      return false;
    }
    return true;
  }

  //fill descriptors array and return actual container size
  uint_t ParseSCLFile(const IO::FastDump& data, FileDescriptions& descrs)
  {
    if (!CheckSCLFile(data))
    {
      return 0;
    }
    const uint_t limit = data.Size();
    const SCLHeader* const header = safe_ptr_cast<const SCLHeader*>(data.Data());

    FileDescriptions res;
    res.reserve(header->BlocksCount);
    std::size_t offset = safe_ptr_cast<const uint8_t*>(header->Blocks + header->BlocksCount) -
                    safe_ptr_cast<const uint8_t*>(header);
    for (uint_t idx = 0; idx != header->BlocksCount; ++idx)
    {
      const SCLEntry& entry = header->Blocks[idx];
      const std::size_t nextOffset = offset + entry.SizeInSectors * BYTES_PER_SECTOR;
      if (nextOffset > limit)
      {
        //file is trunkated
        break;
      }

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
      offset = nextOffset;
    }
    descrs.swap(res);
    return offset;
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

    virtual Error Process(const Parameters::Map& commonParams, 
      const DetectParameters& detectParams,
      const MetaContainer& data, ModuleRegion& region) const
    {
      const IO::FastDump dump(*data.Data);
      FileDescriptions files;
      const uint_t parsedSize = ParseSCLFile(dump, files);
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
        message.Level = CalculateContainersNesting(data.PluginsChain);
        message.Progress = -1;
      }

      MetaContainer subcontainer;
      subcontainer.PluginsChain = data.PluginsChain;
      subcontainer.PluginsChain.push_back(shared_from_this());
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
          message.Text = (SafeFormatter(data.Path.empty() ? Text::PLUGIN_SCL_PROGRESS_NOPATH : Text::PLUGIN_SCL_PROGRESS) % it->Name % data.Path).str();
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

    IO::DataContainer::Ptr Open(const Parameters::Map& /*commonParams*/,
      const MetaContainer& inData, const String& inPath, String& restPath) const
    {
      String restComp;
      const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
      FileDescriptions files;
      if (pathComp.empty() ||
          !ParseSCLFile(IO::FastDump(*inData.Data), files))
      {
        return IO::DataContainer::Ptr();
      }
      const FileDescriptions::const_iterator fileIt = std::find_if(files.begin(), files.end(),
        boost::bind(&TRDFileEntry::Name, _1) == pathComp);
      if (fileIt != files.end())
      {
        restPath = restComp;
        return inData.Data->GetSubcontainer(fileIt->Offset, fileIt->Size);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterSCLContainer(PluginsEnumerator& enumerator)
  {
    const ContainerPlugin::Ptr plugin(new SCLPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
