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

#include <text/common.h>
#include <text/errors.h>
#include <text/plugins.h>

#define FILE_TAG DE9E5D02

namespace
{
  using namespace ZXTune;

  const String TEXT_PACKED_VERSION(FromChar("Revision: $Rev:$"));

  const std::size_t PACKED_MAX_SIZE = 1 << 16;

  //////////////////////////////////////////////////////////////////////////
  class PackedContainer : public ModulePlayer
  {
  public:
    PackedContainer(const String& filename, const IO::DataContainer& data, const String& subcontainers, uint32_t capFilter)
      : Delegate(ModulePlayer::Create(filename, data, capFilter))
      , Subcontainers(subcontainers)
    {
      if (!Delegate.get())
      {
        throw Error(ERROR_DETAIL, 1, TEXT_ERROR_CONTAINER_PLAYER);//TODO: code
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
        ctrIter->second = String(Subcontainers) + TEXT_CONTAINER_DELIMITER + ctrIter->second;
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

    virtual State SetPosition(std::size_t frame)
    {
      assert(Delegate.get());
      return Delegate->SetPosition(frame);
    }

    virtual void Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      assert(Delegate.get());
      return Delegate->Convert(param, dst);
    }

  private:
    ModulePlayer::Ptr Delegate;
    const String Subcontainers;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_STOR_CONTAINER;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PACKED_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PACKED_VERSION));
    StringArray packfmts;
    Archive::GetDepackersList(packfmts);
    OutStringStream str;
    static const String::value_type DELIMITER[] = {' ', 0};
    std::copy(packfmts.begin(), packfmts.end(),
      std::ostream_iterator<String, String::value_type>(str, DELIMITER));
    info.Properties.insert(StringMap::value_type(ATTR_SUBFORMATS, str.str()));
  }

  //checking top-level container
  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    const std::size_t limit(std::min(source.Size(), PACKED_MAX_SIZE));
    String tmp;
    return Archive::Check(source.Data(), limit, tmp);
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
  {
    String subcontainers, container;
    const uint8_t* const ptr(static_cast<const uint8_t*>(data.Data()));
    Dump data1(ptr, ptr + data.Size()), data2;
    //process possibly nested files
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
      throw Error(ERROR_DETAIL, 1, TEXT_ERROR_CONTAINER_EMPTY);//TODO: code
    }
    IO::DataContainer::Ptr asContainer(IO::DataContainer::Create(data1));
    return ModulePlayer::Ptr(new PackedContainer(filename, *asContainer, subcontainers, capFilter));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
