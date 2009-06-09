#include "plugin_enumerator.h"
#include "../io/container.h"

#include <tools.h>

#include <module_attrs.h>
#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <numeric>

namespace
{
  using namespace ZXTune;

  const String TEXT_TS_INFO("TurboSound modules support");
  const String TEXT_TS_VERSION("0.1");
  const String TEXT_TS_CONTAINER("TurboSound");
  const String TEXT_CONTAINER_DELIMITER("=>");

  const std::size_t TS_MAX_SIZE = 1 << 17;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Footer
  {
    uint8_t ID1[4];//'PT3!'
    uint16_t Size1;
    uint8_t ID2[4];//same
    uint16_t Size2;
    uint8_t ID3[4];//'02TS'
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t TS_ID[] = {'0', '2', 'T', 'S'};
  BOOST_STATIC_ASSERT(sizeof(Footer) == 16);

  class TSMixer : public Sound::Receiver
  {
  public:
    TSMixer() : Channels(0), Samples(0), Buffer(), Cursor(0), Receiver(0)
    {
    }

    virtual void ApplySample(const Sound::Sample* input, std::size_t channels)
    {
      if (!Channels)
      {
        Channels = channels;
        Buffer.resize(Channels * Samples);
        Cursor = &Buffer[0];//TODO
      }
      assert(Cursor && Cursor < &Buffer[Buffer.size()]);
      if (Receiver) //mix and out
      {
        std::transform(input, input + channels, Cursor, Cursor, std::max<Sound::Sample>);
        Receiver->ApplySample(Cursor, channels);
      }
      else //store
      {
        std::memcpy(Cursor, input, channels * sizeof(Sound::Sample));
      }
      Cursor += channels;
    }

    virtual void Flush()
    {

    }

    void Reset(const Sound::Parameters& params)
    {
      Receiver = 0;
      Samples = 2 * params.SoundFreq * params.FrameDuration / 1000;
    }

    void Switch(Sound::Receiver& receiver)
    {
      Receiver = &receiver;
      Cursor = &Buffer[0];
    }
  private:
    std::size_t Channels;
    std::size_t Samples;
    std::vector<Sound::Sample> Buffer;
    Sound::Sample* Cursor;
    Sound::Receiver* Receiver;
  };

  //////////////////////////////////////////////////////////////////////////
  class PlayerImpl : public ModulePlayer
  {
  public:
    PlayerImpl(const String& filename, const IO::DataContainer& data)
    {
      //assume all is correct
      const uint8_t* buf(static_cast<const uint8_t*>(data.Data()));
      const Footer* const footer(safe_ptr_cast<const Footer*>(buf + data.Size() - sizeof(Footer)));
      IO::DataContainer::Ptr cont1(data.GetSubcontainer(0, footer->Size1));
      IO::DataContainer::Ptr cont2(data.GetSubcontainer(footer->Size1, footer->Size2));
      Delegate1 = ModulePlayer::Create(filename, *cont1);
      Delegate2 = ModulePlayer::Create(filename, *cont2);
    }
    /// Retrieving player info itself
    virtual void GetInfo(Info& info) const
    {
      assert(Delegate1.get());
      return Delegate1->GetInfo(info);
    }

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const
    {
      assert(Delegate1.get());
      Delegate1->GetModuleInfo(info);
      StringMap::iterator ctrIter(info.Properties.find(Module::ATTR_CONTAINER));
      if (ctrIter == info.Properties.end())
      {
        info.Properties.insert(StringMap::value_type(Module::ATTR_CONTAINER, TEXT_TS_CONTAINER));
      }
      else
      {
        ctrIter->second += TEXT_CONTAINER_DELIMITER;
        ctrIter->second += TEXT_TS_CONTAINER;
      }
    }

    /// Retrieving current state of loaded module
    virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
    {
      assert(Delegate1.get());
      return Delegate1->GetModuleState(timeState, trackState);
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      assert(Delegate1.get() && Delegate2.get());
      Sound::Analyze::ChannelsState state2;
      const State result(std::min(Delegate1->GetSoundState(state), Delegate2->GetSoundState(state2)));
      state.insert(state.end(), state2.begin(), state2.end());
      return result;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      assert(Delegate1.get() && Delegate2.get());
      Mixer.Reset(params);
      const State state1(Delegate1->RenderFrame(params, Mixer));
      Mixer.Switch(receiver);
      const State state2(Delegate2->RenderFrame(params, Mixer));
      return std::min(state1, state2);
    }

    /// Controlling
    virtual State Reset()
    {
      assert(Delegate1.get() && Delegate2.get());
      return std::min(Delegate1->Reset(), Delegate2->Reset());
    }

    virtual State SetPosition(const uint32_t& frame)
    {
      assert(Delegate1.get() && Delegate2.get());
      return std::min(Delegate1->SetPosition(frame), Delegate2->SetPosition(frame));
    }
  private:
    ModulePlayer::Ptr Delegate1;
    ModulePlayer::Ptr Delegate2;
    TSMixer Mixer;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_CONTAINER;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_TS_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_TS_VERSION));
  }

  //checking top-level container
  bool Checking(const String& filename, const IO::DataContainer& source)
  {
    const std::size_t limit(source.Size());
    if (limit >= TS_MAX_SIZE || limit < sizeof(Footer))
    {
      return false;
    }
    const uint8_t* buf(static_cast<const uint8_t*>(source.Data()));
    const Footer* const footer(safe_ptr_cast<const Footer*>(buf + limit - sizeof(Footer)));
    return 0 == std::memcmp(footer->ID3, TS_ID, sizeof(TS_ID)) &&
        footer->Size1 < limit && footer->Size2 < limit && 
        footer->Size1 + footer->Size2 + sizeof(*footer) == limit;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data)
  {
    assert(Checking(filename, data) || !"Attempt to create TS player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator tsReg(Checking, Creating, Describing);
}
