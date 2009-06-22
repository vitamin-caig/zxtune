#include "plugin_enumerator.h"
#include "../io/container.h"

#include <error.h>
#include <tools.h>

#include <module_attrs.h>
#include <player_attrs.h>

#include "../../archives/archive.h"

#include <boost/static_assert.hpp>

#include <cassert>
#include <numeric>

#define FILE_TAG DE9E5D02

namespace
{
  using namespace ZXTune;

  const String TEXT_PCK_INFO("Packed modules support");
  const String TEXT_PCK_VERSION("0.1");
  const String TEXT_CONTAINER_DELIMITER("=>");

  const std::size_t PACKED_MAX_SIZE = 1 << 16;

  //////////////////////////////////////////////////////////////////////////
  class PlayerImpl : public ModulePlayer
  {
  public:
    PlayerImpl(const String& filename, const IO::DataContainer& data, const String& subcontainers)
      : Delegate(ModulePlayer::Create(filename, data))
      , Subcontainers(subcontainers)
    {
      if (!Delegate.get())
      {
        throw Error(ERROR_DETAIL, 1);//TODO
      }
    }
    /// Retrieving player info itself
    virtual void GetInfo(Info& info) const
    {
      assert(Delegate.get());
      return Delegate->GetInfo(info);
    }

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const
    {
      assert(Delegate.get());
      Delegate->GetModuleInfo(info);
      StringMap::iterator ctrIter(info.Properties.find(Module::ATTR_CONTAINER));
      if (ctrIter == info.Properties.end())
      {
        info.Properties.insert(StringMap::value_type(Module::ATTR_CONTAINER, Subcontainers));
      }
      else
      {
        ctrIter->second += TEXT_CONTAINER_DELIMITER;
        ctrIter->second += Subcontainers;
      }
    }

    /// Retrieving current state of loaded module
    virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
    {
      assert(Delegate.get());
      return Delegate->GetModuleState(timeState, trackState);
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      assert(Delegate.get());
      return Delegate->GetSoundState(state);
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      assert(Delegate.get());
      return Delegate->RenderFrame(params, receiver);
    }

    /// Controlling
    virtual State Reset()
    {
      assert(Delegate.get());
      return Delegate->Reset();
    }

    virtual State SetPosition(const uint32_t& frame)
    {
      assert(Delegate.get());
      return Delegate->SetPosition(frame);
    }
  private:
    ModulePlayer::Ptr Delegate;
    const String Subcontainers;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_CONTAINER;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PCK_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PCK_VERSION));
  }

  //checking top-level container
  bool Checking(const String& filename, const IO::DataContainer& source)
  {
    const std::size_t limit(source.Size());
    if (limit >= PACKED_MAX_SIZE)
    {
      return false;
    }
    String tmp;
    return Archive::Check(source.Data(), limit, tmp);
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data)
  {
    String subcontainers, container;
    const uint8_t* const ptr(static_cast<const uint8_t*>(data.Data()));
    Dump data1(ptr, ptr + data.Size()), data2;
    while (Archive::Check(&data1[0], data1.size(), container) && 
      Archive::Depack(&data1[0], data1.size(), data2, container))
    {
      if (!subcontainers.empty())
      {
        subcontainers += TEXT_CONTAINER_DELIMITER;
      }
      subcontainers += container;
      data1.swap(data2);
    }
    if (subcontainers.empty())
    {
      throw Error(ERROR_DETAIL, 1);//TODO
    }
    IO::DataContainer::Ptr asContainer(IO::DataContainer::Create(data1));
    return ModulePlayer::Ptr(new PlayerImpl(filename, *asContainer, subcontainers));
  }

  PluginAutoRegistrator pckReg(Checking, Creating, Describing);
}
