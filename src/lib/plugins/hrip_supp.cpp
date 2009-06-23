#include "plugin_enumerator.h"

#include "../io/container.h"
#include "../io/location.h"
#include "../../archives/formats/hrip.h"

#include <tools.h>
#include <error.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cctype>
#include <cassert>
#include <valarray>

#define FILE_TAG 942A64CC

namespace
{
  using namespace ZXTune;

  const String TEXT_HRP_INFO("HRiP modules support");
  const String TEXT_HRP_VERSION("0.1");

  const std::size_t HRP_MODULE_SIZE = 655360;

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info);

  class PlayerImpl : public ModulePlayer
  {
  public:
    PlayerImpl(const String& filename, const IO::DataContainer& data) : Filename(filename)
    {
      StringArray pathes;
      IO::SplitPath(filename, pathes);
      if (1 == pathes.size())
      {
        //enumerate
        String submodules;
        Archive::HripFiles files;
        Archive::CheckHrip(data.Data(), data.Size(), files);
        //TODO: merge
        for (Archive::HripFiles::const_iterator it = files.begin(), lim = files.end(); it != lim; ++it)
        {
          const String& modPath(IO::CombinePath(filename, it->Filename));
          Dump dump;
          if (Archive::OK != Archive::DepackHrip(data.Data(), data.Size(), it->Filename, dump))
          {
            throw 1;//TODO
          }
          IO::DataContainer::Ptr container(IO::DataContainer::Create(dump));
          ModulePlayer::Ptr tmp(ModulePlayer::Create(modPath, *container));
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
        Dump dump;
        if (Archive::OK != Archive::DepackHrip(data.Data(), data.Size(), pathes[1], dump))
        {
          throw 1;//TODO
        }
        IO::DataContainer::Ptr subContainer(IO::DataContainer::Create(dump));
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
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_HRP_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_HRP_VERSION));
  }

  //checking top-level container
  bool Checking(const String& /*filename*/, const IO::DataContainer& source)
  {
    const std::size_t limit(source.Size());
    if (limit >= HRP_MODULE_SIZE)
    {
      return false;
    }
    Archive::HripFiles files;
    return Archive::OK == Archive::CheckHrip(source.Data(), source.Size(), files) && !files.empty();
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data)
  {
    assert(Checking(filename, data) || !"Attempt to create hrp player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator hrpReg(Checking, Creating, Describing);
}
