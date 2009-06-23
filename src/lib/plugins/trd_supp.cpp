#include "plugin_enumerator.h"

#include "../io/container.h"
#include "../io/location.h"
#include "../io/trdos.h"

#include <tools.h>
#include <error.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cctype>
#include <cassert>
#include <valarray>

#define FILE_TAG A1239034

namespace
{
  using namespace ZXTune;

  const String TEXT_TRD_INFO("TRD modules support");
  const String TEXT_TRD_VERSION("0.1");

  const std::size_t TRD_MODULE_SIZE = 655360;
  const std::size_t BYTES_PER_SECTOR = 256;
  const std::size_t SECTORS_IN_TRACK = 16;
  const std::size_t MAX_FILES_COUNT = 128;
  const std::size_t SERVICE_SECTOR_NUM = 8;

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
    IO::TRDosName Name;
    uint16_t Length;
    uint8_t SizeInSectors;
    uint8_t Sector;
    uint8_t Track;

    std::size_t Offset() const
    {
      return BYTES_PER_SECTOR * (SECTORS_IN_TRACK * Track + Sector);
    }
  } PACK_POST;

  enum
  {
    TRD_ID = 0x10,

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

  struct FileDescr
  {
    explicit FileDescr(const CatEntry& entry)
     : Name(GetFileName(entry.Name))
     , Offset(entry.Offset())
     , Size(entry.SizeInSectors == ((entry.Length - 1) / BYTES_PER_SECTOR) ?
      entry.Length : BYTES_PER_SECTOR * entry.SizeInSectors)
    {
    }

    bool IsMergeable(const CatEntry& rh)
    {
      return Size == 255 * BYTES_PER_SECTOR && Offset + Size == rh.Offset();
    }

    void Merge(const CatEntry& rh)
    {
      assert(IsMergeable(rh));
      Size += rh.Length;
    }

    bool operator == (const String& name) const
    {
      return Name == name;
    }

    String Name;
    std::size_t Offset;
    std::size_t Size;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info);

  class PlayerImpl : public ModulePlayer
  {
  public:
    PlayerImpl(const String& filename, const IO::DataContainer& data) : Filename(filename)
    {
      std::vector<FileDescr> files;
      const CatEntry* catEntry(static_cast<const CatEntry*>(data.Data()));
      for (std::size_t idx = 0; idx != MAX_FILES_COUNT && NOENTRY != *catEntry->Name.Filename; ++idx, ++catEntry)
      {
        if (DELETED != *catEntry->Name.Filename && catEntry->SizeInSectors)
        {
          if (files.empty() || !files.back().IsMergeable(*catEntry))
          {
            files.push_back(FileDescr(*catEntry));
          }
          else
          {
            files.back().Merge(*catEntry);
          }
        }
      }

      StringArray pathes;
      IO::SplitPath(filename, pathes);
      if (1 == pathes.size())
      {
        //enumerate
        String submodules;
        for (std::vector<FileDescr>::const_iterator it = files.begin(), lim = files.end(); it != lim; ++it)
        {
          const String& modPath(IO::CombinePath(filename, it->Name));
          IO::DataContainer::Ptr subContainer(data.GetSubcontainer(it->Offset, it->Size));
          ModulePlayer::Ptr tmp(ModulePlayer::Create(modPath, *subContainer));
          if (tmp.get())//detected module
          {
            ModulePlayer::Info info;
            tmp->GetInfo(info);
            if (info.Capabilities & CAP_MULTITRACK)
            {
              Module::Information modInfo;
              tmp->GetModuleInfo(modInfo);
              submodules += modInfo.Properties[Module::ATTR_SUBMODULES];
            }
            else
            {
              if (submodules.empty())
              {
                submodules = modPath;
              }
              else
              {
                submodules += '\n';//TODO
                submodules += modPath;
              }
            }
          }
        }
        if (submodules.empty())
        {
          throw Error(ERROR_DETAIL, 1);//TODO: no files
        }
        Information.Capabilities = CAP_MULTITRACK;
        Information.Loop = 0;
        Information.Statistic = Module::Tracking();
        Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, Filename));
        Information.Properties.insert(StringMap::value_type(Module::ATTR_SUBMODULES, submodules));
      }
      else
      {
        //open existing
        std::vector<FileDescr>::const_iterator iter(std::find(files.begin(), files.end(), pathes[1]));
        if (files.end() == iter)
        {
          throw Error(ERROR_DETAIL, 1);//TODO: invalid file
        }
        IO::DataContainer::Ptr subContainer(data.GetSubcontainer(iter->Offset, iter->Size));
        Delegate = ModulePlayer::Create(IO::ExtractSubpath(filename), *subContainer);
        if (!Delegate.get())
        {
          throw Error(ERROR_DETAIL, 1);//TODO: invalid file
        }
      }
    }

    /// Retrieving player info itself
    virtual void GetInfo(Info& info) const
    {
      if (Delegate.get())
      {
        Delegate->GetInfo(info);
      }
      else
      {
        Describing(info);
      }
    }

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const
    {
      if (Delegate.get())
      {
        Delegate->GetModuleInfo(info);
        StringMap::iterator filenameIt(info.Properties.find(Module::ATTR_FILENAME));
        if (filenameIt != info.Properties.end())
        {
          filenameIt->second = Filename;
        }
        else
        {
          assert(!"No filename properties");
        }
      }
      else
      {
        info = Information;
      }
    }

    /// Retrieving current state of loaded module
    virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
    {
      return Delegate.get() ? Delegate->GetModuleState(timeState, trackState) : MODULE_STOPPED;
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      return Delegate.get() ? Delegate->GetSoundState(state) : MODULE_STOPPED;
    }


    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      return Delegate.get() ? Delegate->RenderFrame(params, receiver) : MODULE_STOPPED;
    }

    /// Controlling
    virtual State Reset()
    {
      return Delegate.get() ? Delegate->Reset() : MODULE_STOPPED;
    }

    virtual State SetPosition(const uint32_t& frame)
    {
      return Delegate.get() ? Delegate->SetPosition(frame) : MODULE_STOPPED;
    }
  private:
    const String Filename;
    ModulePlayer::Ptr Delegate;
    Module::Information Information;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_MULTITRACK;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_TRD_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_TRD_VERSION));
  }

  //checking top-level container
  bool Checking(const String& /*filename*/, const IO::DataContainer& source)
  {
    const std::size_t limit(source.Size());
    if (limit != TRD_MODULE_SIZE)
    {
      return false;
    }
    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    const ServiceSector* const sector(safe_ptr_cast<const ServiceSector*>(data + SERVICE_SECTOR_NUM * BYTES_PER_SECTOR));
    return sector->ID == TRD_ID && sector->Type == DS_DD;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data)
  {
    assert(Checking(filename, data) || !"Attempt to create trd player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator trdReg(Checking, Creating, Describing);
}
