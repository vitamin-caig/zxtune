#include "plugin_enumerator.h"

#include "../io/container.h"
#include "../io/location.h"

#include <error.h>
#include <tools.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>

#define FILE_TAG 4100118E

namespace
{
  using namespace ZXTune;

  const String TEXT_RAW_INFO("RAW modules scanner");
  const String TEXT_RAW_VERSION("0.1");
  const String TEXT_RAW_CONTAINER("Raw");
  const String TEXT_CONTAINER_DELIMITER("=>");

  const std::size_t MAX_MODULE_SIZE = 1048576;//1Mb
  const std::size_t SCAN_STEP = 256;
  const std::size_t MIN_SCAN_SIZE = 512;

  const String::value_type EXTENTION[] = {'.', 'r', 'a', 'w', '\0'};

  String MakeFilename(std::size_t offset)
  {
    OutStringStream str;
    str << offset << EXTENTION;
    return str.str();
  }

  bool ParseFilename(const String& filename, std::size_t& offset)
  {
    InStringStream str(filename);
    String ext;
    return (str >> offset) && (str >> ext) && ext == EXTENTION && 0 == (offset % SCAN_STEP);
  }

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info);

  class PlayerImpl : public ModulePlayer
  {
  public:
    PlayerImpl(const String& filename, const IO::DataContainer& data) : Filename(filename)
    {
      const std::size_t limit(data.Size());
      StringArray pathes;
      IO::SplitPath(filename, pathes);
      std::size_t offset(0);
      if (1 == pathes.size() || !ParseFilename(pathes.back(), offset))
      {
        //enumerate
        String submodules;
        for (std::size_t off = 0; off < limit - MIN_SCAN_SIZE; off += SCAN_STEP)
        {
          const String& modPath(IO::CombinePath(filename, MakeFilename(off)));
          ModulePlayer::Ptr tmp(ModulePlayer::Create(modPath, *data.GetSubcontainer(off, limit - off)));
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
          throw Error(ERROR_DETAIL, 1);//TODO
        }
        Information.Capabilities = CAP_SCANER;
        Information.Loop = 0;
        Information.Statistic = Module::Tracking();
        Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, Filename));
        Information.Properties.insert(StringMap::value_type(Module::ATTR_SUBMODULES, submodules));
      }
      else
      {
        //open existing
        Delegate = ModulePlayer::Create(IO::ExtractSubpath(filename), *data.GetSubcontainer(offset, limit - offset));
        if (!Delegate.get())
        {
          throw 1;//TODO: invalid file
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
        /*
        StringMap::iterator filenameIt(info.Properties.find(Module::ATTR_FILENAME));
        if (filenameIt != info.Properties.end())
        {
          filenameIt->second = Filename;
        }
        else
        {
          assert(!"No filename properties");
        }
        */
        StringMap::iterator ctrIter(info.Properties.find(Module::ATTR_CONTAINER));
        if (ctrIter == info.Properties.end())
        {
          info.Properties.insert(StringMap::value_type(Module::ATTR_CONTAINER, TEXT_RAW_CONTAINER));
        }
        else
        {
          ctrIter->second += TEXT_CONTAINER_DELIMITER;
          ctrIter->second += TEXT_RAW_CONTAINER;
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
    info.Capabilities = CAP_SCANER;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_RAW_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_RAW_VERSION));
  }

  //checking top-level container
  bool Checking(const String& filename, const IO::DataContainer& source)
  {
    const std::size_t limit(source.Size());
    StringArray pathes;
    IO::SplitPath(filename, pathes);
    std::size_t offset(0);
    if (limit >= MIN_SCAN_SIZE && limit <= MAX_MODULE_SIZE &&
      (1 == pathes.size() || !ParseFilename(pathes.back(), offset)))
    {
      unsigned modules(0);
      for (std::size_t off = 0; off < limit - MIN_SCAN_SIZE; off += SCAN_STEP)
      {
        const String& modPath(IO::CombinePath(filename, MakeFilename(off)));
        ModulePlayer::Info info;
        if (ModulePlayer::Check(modPath, *source.GetSubcontainer(off, limit - off), info))
        {
          ++modules;
        }
      }
      return modules > 1;//TODO: duplicated module with aligned players
    }
    return false;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data)
  {
    assert(Checking(filename, data) || !"Attempt to create raw player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator rawReg(Checking, Creating, Describing);
}
