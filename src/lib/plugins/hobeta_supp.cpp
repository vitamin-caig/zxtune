#include "plugin_enumerator.h"
#include "../io/container.h"

#include <tools.h>
#include <error.h>

#include <module_attrs.h>
#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <numeric>

#define FILE_TAG 1CF1A62A

namespace
{
  using namespace ZXTune;

  const String TEXT_HOB_INFO("Hobeta modules support");
  const String TEXT_HOB_VERSION("0.1");
  const String TEXT_HOB_CONTAINER("Hobeta");
  const String TEXT_CONTAINER_DELIMITER("=>");

  const std::size_t HOBETA_MAX_SIZE = 0xff00 + 17;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Header
  {
    uint8_t Filename[8];
    uint8_t Filetype[3];
    uint16_t Length;
    uint8_t Zero;
    uint8_t SizeInSectors;
    uint16_t CRC;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Header) == 17);

  //////////////////////////////////////////////////////////////////////////
  class PlayerImpl : public ModulePlayer
  {
  public:
    PlayerImpl(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
     : Delegate(ModulePlayer::Create(filename, *data.GetSubcontainer(sizeof(Header), data.Size() - sizeof(Header)), capFilter))
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
        info.Properties.insert(StringMap::value_type(Module::ATTR_CONTAINER, TEXT_HOB_CONTAINER));
      }
      else
      {
        ctrIter->second = String(TEXT_HOB_CONTAINER) + TEXT_CONTAINER_DELIMITER + ctrIter->second;
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
  private:
    ModulePlayer::Ptr Delegate;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_CONTAINER;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_HOB_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_HOB_VERSION));
  }

  //checking top-level container
  bool Checking(const String& filename, const IO::DataContainer& source, uint32_t capFilter)
  {
    const std::size_t limit(source.Size());
    if (limit >= HOBETA_MAX_SIZE)
    {
      return false;
    }
    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    const Header* const header(safe_ptr_cast<const Header*>(data));
    ModulePlayer::Info info;
    IO::DataContainer::Ptr subcontainer(source.GetSubcontainer(sizeof(*header),
      source.Size() - sizeof(*header)));
    return fromLE(header->CRC) == ((105 + 257 * std::accumulate(data, data + 15, 0)) & 0xffff) &&
           ModulePlayer::Check(filename, *subcontainer, info, capFilter);
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t capFilter)
  {
    assert(Checking(filename, data, capFilter) || !"Attempt to create hobeta player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data, capFilter));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
