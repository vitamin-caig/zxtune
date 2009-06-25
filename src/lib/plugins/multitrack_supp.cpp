#include "multitrack_supp.h"

#include "../io/location.h"

#include <player_attrs.h>

#include <error.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include <set>

#define FILE_TAG 509C5C50

namespace
{
  const String::value_type TEXT_CONTAINER_DELIMITER[] = {'=', '>', 0};

  void Explode(const String& asString, StringArray& asArray)
  {
    boost::algorithm::split(asArray, asString, boost::algorithm::is_cntrl());
  }

  String Join(const StringArray& asArray)
  {
    String result;
    for (StringArray::const_iterator it = asArray.begin(), lim = asArray.end(); it != lim; ++it)
    {
      if (it != asArray.begin())
      {
        result += '\n';//TODO
      }
      result += *it;
    }
    return result;
  }
}

namespace ZXTune
{
  MultitrackBase::MultitrackBase(const String& selfName, const String& id) : Filename(selfName), Id(id)
  {
  }

  bool MultitrackBase::GetPlayerInfo(Info& info) const
  {
    if (Delegate.get())
    {
      Delegate->GetInfo(info);
      return true;
    }
    return false;
  }

  void MultitrackBase::GetModuleInfo(Module::Information& info) const
  {
    if (Delegate.get())
    {
      Delegate->GetModuleInfo(info);
      StringMap::iterator propIt(info.Properties.find(Module::ATTR_FILENAME));
      if (propIt != info.Properties.end())
      {
        propIt->second = Filename;
      }
      else
      {
        assert(!"Invalid case");
      }
      propIt = info.Properties.find(Module::ATTR_CONTAINER);
      if (propIt != info.Properties.end())
      {
        propIt->second = Id + TEXT_CONTAINER_DELIMITER + propIt->second;
      }
      else
      {
        info.Properties.insert(StringMap::value_type(Module::ATTR_CONTAINER, Id));
      }
    }
    else
    {
      info = Information;
    }
  }

  ModulePlayer::State MultitrackBase::GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
  {
    return Delegate.get() ? Delegate->GetModuleState(timeState, trackState) : MODULE_STOPPED;
  }

  ModulePlayer::State MultitrackBase::GetSoundState(Sound::Analyze::ChannelsState& state) const
  {
    return Delegate.get() ? Delegate->GetSoundState(state) : MODULE_STOPPED;
  }

  ModulePlayer::State MultitrackBase::RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
  {
    return Delegate.get() ? Delegate->RenderFrame(params, receiver) : MODULE_STOPPED;
  }

  ModulePlayer::State MultitrackBase::Reset()
  {
    return Delegate.get() ? Delegate->Reset() : MODULE_STOPPED;
  }

  ModulePlayer::State MultitrackBase::SetPosition(std::size_t frame)
  {
    return Delegate.get() ? Delegate->SetPosition(frame) : MODULE_STOPPED;
  }

  void MultitrackBase::Convert(const Conversion::Parameter& param, Dump& dst) const
  {
    if (Delegate.get())
    {
      return Delegate->Convert(param, dst);
    }
    throw Error(ERROR_DETAIL, 1);//TODO
  }

  void MultitrackBase::Process(SubmodulesIterator& iterator, uint32_t capFilter)
  {
    StringArray pathes;
    IO::SplitPath(Filename, pathes);
    std::set<String> uniques;
    if (1 == pathes.size()) //enumerate
    {
      StringArray submodules;
      String file;
      IO::DataContainer::Ptr container;
      for (iterator.Reset(); iterator.Get(file, container); iterator.Next())
      {
        ModulePlayer::Ptr tmp(ModulePlayer::Create(file, *container, capFilter));
        if (tmp.get())//detected module
        {
          Module::Information modInfo;
          tmp->GetModuleInfo(modInfo);

          ModulePlayer::Info info;
          tmp->GetInfo(info);
          if (info.Capabilities & CAP_STOR_MULTITRACK)
          {
            StringArray subsubmodules;
            Explode(modInfo.Properties[Module::ATTR_SUBMODULES], subsubmodules);
            std::transform(subsubmodules.begin(), subsubmodules.end(), std::back_inserter(submodules), 
              boost::bind(static_cast<String (*)(const String&, const String&)>(&IO::CombinePath), 
                Filename, _1));
          }
          else
          {
            const String& crc(modInfo.Properties[Module::ATTR_CRC]);
            if (crc.empty() || uniques.insert(crc).second)
            {
              submodules.push_back(IO::CombinePath(Filename, file));
            }
          }
        }
      }
      if (submodules.empty())
      {
        throw Error(ERROR_DETAIL, 1);//TODO
      }
      Information.Capabilities = CAP_STOR_MULTITRACK;
      Information.Loop = 0;
      Information.Statistic = Module::Tracking();
      Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, Filename));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_SUBMODULES, Join(submodules)));
    }
    else
    {
      iterator.Reset(pathes[1]);
      String file;
      IO::DataContainer::Ptr container;
      if (iterator.Get(file, container))
      {
        Delegate = ModulePlayer::Create(IO::ExtractSubpath(Filename), *container, capFilter);
      }
      if (!Delegate.get())
      {
        throw Error(ERROR_DETAIL, 1);//TODO
      }
    }
  }
}
