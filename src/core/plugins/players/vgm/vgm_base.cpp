/**
 *
 * @file
 *
 * @brief  libvgm-based formats support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/vgm/videogamemusic.h"
#include "formats/chiptune/multidevice/sound98.h"
#include "formats/chiptune/multidevice/videogamemusic.h"
#include "module/players/duration.h"
#include "module/players/platforms.h"
#include "module/players/properties_helper.h"
#include "module/players/properties_meta.h"
#include "module/players/streaming.h"

#include "binary/container_factories.h"
#include "core/core_parameters.h"
#include "core/plugin_attrs.h"
#include "debug/log.h"
#include "math/numeric.h"
#include "module/attributes.h"
#include "parameters/tracking_helper.h"
#include "tools/xrange.h"

#include "contract.h"
#include "error_tools.h"
#include "make_ptr.h"

#include "3rdparty/vgm/emu/SoundEmu.h"
#include "3rdparty/vgm/player/playera.hpp"
#include "3rdparty/vgm/player/s98player.hpp"
#include "3rdparty/vgm/player/vgmplayer.hpp"
#include "3rdparty/vgm/utils/DataLoader.h"

#include <map>

namespace Module::LibVGM
{
  const Debug::Stream Dbg("Core::VGMSupp");

  using PlayerPtr = std::unique_ptr< ::PlayerA>;

  using PlayerCreator = PlayerPtr (*)();

  template<class PlayerType>
  PlayerPtr Create()
  {
    auto player = PlayerPtr(new PlayerA());
    player->RegisterPlayerEngine(new PlayerType());
    return player;
  }

  struct Model
  {
    using Ptr = std::shared_ptr<Model>;

    Model(PlayerCreator create, Binary::View data)
      : CreatePlayer(create)
      , Data(Binary::CreateContainer(data))
    {}

    const PlayerCreator CreatePlayer;
    const Binary::Data::Ptr Data;
  };

  class LoaderAdapter
  {
  public:
    explicit LoaderAdapter(Binary::View raw)
      : Raw(raw)
    {
      std::memset(&Delegate, 0, sizeof(Delegate));
      static const DATA_LOADER_CALLBACKS CALLBACKS = {0xdeadbeef, "",    &Open,   &Read, nullptr,
                                                      &Close,     &Tell, &Length, &Eof,  nullptr};
      ::DataLoader_Setup(&Delegate, &CALLBACKS, this);
      Require(0 == ::DataLoader_Load(Get()));
    }

    ~LoaderAdapter()
    {
      ::DataLoader_Reset(&Delegate);
    }

    DATA_LOADER* Get()
    {
      return &Delegate;
    }

  private:
    static LoaderAdapter* Cast(void* ctx)
    {
      return static_cast<LoaderAdapter*>(ctx);
    }

    static UINT8 Open(void* ctx)
    {
      Cast(ctx)->Position = 0;
      return 0;
    }

    static UINT32 Read(void* ctx, UINT8* buf, UINT32 size)
    {
      auto* self = Cast(ctx);
      if (const auto sub = self->Raw.SubView(self->Position, size))
      {
        std::memcpy(buf, sub.Start(), sub.Size());
        self->Position += sub.Size();
        return sub.Size();
      }
      else
      {
        return 0;
      }
    }

    static UINT8 Close(void* /*ctx*/)
    {
      return 0;
    }

    static INT32 Tell(void* ctx)
    {
      return Cast(ctx)->Position;
    }

    static UINT32 Length(void* ctx)
    {
      return Cast(ctx)->Raw.Size();
    }

    static UINT8 Eof(void* ctx)
    {
      const auto* self = Cast(ctx);
      return self->Position >= self->Raw.Size();
    }

  private:
    const Binary::View Raw;
    DATA_LOADER Delegate;
    std::size_t Position;
  };

  class ChannelsLayout
  {
    ChannelsLayout(const ChannelsLayout&) = delete;
    ChannelsLayout(ChannelsLayout&&) = default;

  public:
    using Ptr = std::shared_ptr<const ChannelsLayout>;

    explicit ChannelsLayout(const Model& tune)
    {
      LoaderAdapter loader(*tune.Data);
      const auto player = tune.CreatePlayer();
      Require(0 == player->SetOutputSettings(44100, Sound::Sample::CHANNELS, Sound::Sample::BITS, 44100 / 100));
      Require(0 == player->LoadFile(loader.Get()));
      player->Start();  // required so that FindDeviceCore() works
      std::vector<PLR_DEV_INFO> devices;
      player->GetPlayer()->GetSongDeviceInfo(devices);
      Devices.reserve(devices.size());
      for (const auto& dev : devices)
      {
        Devices.emplace_back(dev, TotalChannels);
        auto& curDev = Devices.back();
        TotalChannels += curDev.Core->channels;
        ++Types[dev.type];
      }
    }

    Strings::Array GetChannelsNames() const
    {
      if (TotalChannels > 63)
      {
        return {};
      }
      Strings::Array result;
      result.reserve(TotalChannels);
      for (const auto& dev : Devices)
      {
        String name = dev.Core->name;
        if (Types[dev.Info.type] > 1)
        {
          name += ':';
          name += std::to_string(int(dev.Info.instance));
        }
        if (dev.Core->channels > 1)
        {
          name += '.';
          for (uint_t ch : xrange(dev.Core->channels))
          {
            result.emplace_back(name + std::to_string(ch));
          }
        }
        else
        {
          result.emplace_back(std::move(name));
        }
      }
      return result;
    }

    void ApplyMuteMask(int64_t mask, ::PlayerA& player) const
    {
      for (const auto& dev : Devices)
      {
        PLR_MUTE_OPTS opt = {};
        opt.chnMute[0] = static_cast<UINT32>(mask >> dev.MuteMaskShift);
        player.GetPlayer()->SetDeviceMuting(dev.Info.id, opt);
      }
    }

  private:
    static const DEV_DEF* FindDeviceCore(const PLR_DEV_INFO& dev)
    {
      if (const auto** cores = ::SndEmu_GetDevDefList(dev.type))
      {
        for (const auto* core = *cores; core; core = *++cores)
        {
          if (core->coreID == dev.core)
          {
            return core;
          }
        }
      }
      Require(false);
      return {};
    }

    struct Device
    {
      PLR_DEV_INFO Info;
      const DEV_DEF* Core;
      uint_t MuteMaskShift;

      Device(const PLR_DEV_INFO& info, uint_t shift)
        : Info(info)
        , Core(FindDeviceCore(info))
        , MuteMaskShift(shift)
      {}
    };
    std::vector<Device> Devices;
    uint_t TotalChannels = 0;
    std::array<uint8_t, 0x30> Types = {};
  };

  const Time::Milliseconds FRAME_DURATION(20);

  class VGMEngine : public State
  {
  public:
    using RWPtr = std::shared_ptr<VGMEngine>;

    VGMEngine(Model::Ptr tune, const Module::Information& info, ChannelsLayout::Ptr channels, uint_t samplerate)
      : Tune(std::move(tune))
      , Channels(std::move(channels))
      , Loader(*Tune->Data)
      , Delegate(Tune->CreatePlayer())
    {
      {
        ::PlayerA::Config pCfg = Delegate->GetConfiguration();
        pCfg.masterVol = 0x10000;  // volume 1.0
        pCfg.loopCount = 0;        // infinite looping
        pCfg.fadeSmpls = 0;        // no fade out
        pCfg.endSilenceSmpls = 0;  // no silence at the end
        pCfg.pbSpeed = 1.0;        // normal playback speed
        Delegate->SetConfiguration(pCfg);
      }
      {
        const auto frame_samples = ToSample(FRAME_DURATION, samplerate);
        Require(
            0 == Delegate->SetOutputSettings(samplerate, Sound::Sample::CHANNELS, Sound::Sample::BITS, frame_samples));
      }
      Require(0 == Delegate->LoadFile(Loader.Get()));
      Delegate->Start();
      TotalTicks = ToTicks(info.Duration());
      LoopTicks = ToTicks(info.LoopDuration());
    }

    Time::AtMillisecond At() const override
    {
      // time position in file
      const auto curtime = Delegate->GetCurTime(PLAYTIME_LOOP_EXCL | PLAYTIME_TIME_FILE);
      return Time::AtMillisecond() + Time::Milliseconds(curtime * 1000);
    }

    Time::Milliseconds Total() const override
    {
      // total played time
      const auto curtime = Delegate->GetCurTime(PLAYTIME_LOOP_INCL | PLAYTIME_TIME_PBK);
      return Time::Seconds(curtime * 1000);
    }

    uint_t LoopCount() const override
    {
      // Tracks can specify LoopTicks == 0 to indicate no loop.
      // In this case, we want to loop the whole song.
      return std::max(WholeLoopCount, Delegate->GetCurLoop());
    }

    void Reset()
    {
      Delegate->Reset();
      WholeLoopCount = 0;
    }

    Sound::Chunk Render()
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");

      const auto samples = ToSample(FRAME_DURATION);
      Sound::Chunk result(samples);
      const auto outBytes = Delegate->Render(samples * sizeof(result.front()), result.data());
      result.resize(outBytes / sizeof(result.front()));
      CheckForWholeLoop();
      return result;
    }

    void Seek(Time::AtMillisecond request)
    {
      const auto samples = ToSample(request);
      Require(0 == Delegate->Seek(PLAYPOS_SAMPLE, samples));
      WholeLoopCount = 0;
    }

    void MuteChannels(int64_t mask)
    {
      Channels->ApplyMuteMask(mask, *Delegate);
    }

  private:
    template<class Duration>
    uint_t ToTicks(Duration dur) const
    {
      const auto sample = ToSample(dur);
      return Delegate->GetPlayer()->Sample2Tick(sample);
    }

    template<class Item>
    uint_t ToSample(Item it, uint_t samplerate) const
    {
      return uint_t(uint64_t(it.Get()) * samplerate / Item::PER_SECOND);
    }

    template<class Item>
    uint_t ToSample(Item it) const
    {
      return ToSample(it, Delegate->GetSampleRate());
    }

    void CheckForWholeLoop()
    {
      if (0 != (Delegate->GetState() & PLAYSTATE_END))
      {
        ++WholeLoopCount;
        Require(0 == Delegate->Seek(PLAYPOS_TICK, 0));
      }
    }

  private:
    const Model::Ptr Tune;
    const ChannelsLayout::Ptr Channels;
    LoaderAdapter Loader;
    PlayerPtr Delegate;
    uint_t TotalTicks = 0;
    uint_t LoopTicks = 0;
    uint_t WholeLoopCount = 0;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr tune, const Module::Information& info, ChannelsLayout::Ptr channels, uint_t samplerate,
             Parameters::Accessor::Ptr params)
      : Engine(MakeRWPtr<VGMEngine>(std::move(tune), info, std::move(channels), samplerate))
      , Params(std::move(params))
    {}

    State::Ptr GetState() const override
    {
      return Engine;
    }

    Sound::Chunk Render() override
    {
      ApplyParameters();
      return Engine->Render();
    }

    void Reset() override
    {
      try
      {
        Engine->Reset();
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      try
      {
        Engine->Seek(request);
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

  private:
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Core;
        const auto mask = Parameters::GetInteger(*Params, CHANNELS_MASK, CHANNELS_MASK_DEFAULT);
        Engine->MuteChannels(mask);
      }
    }

  private:
    const VGMEngine::RWPtr Engine;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr tune, Module::Information::Ptr info, ChannelsLayout::Ptr channels,
           Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Info(std::move(info))
      , Channels(std::move(channels))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      try
      {
        return MakePtr<Renderer>(Tune, *Info, Channels, samplerate, std::move(params));
      }
      catch (const std::exception& e)
      {
        throw Error(THIS_LINE, e.what());
      }
    }

  private:
    const Model::Ptr Tune;
    const Module::Information::Ptr Info;
    const ChannelsLayout::Ptr Channels;
    const Parameters::Accessor::Ptr Properties;
  };
}  // namespace Module::LibVGM

namespace Module::VideoGameMusic
{
  class DataBuilder : public Formats::Chiptune::VideoGameMusic::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetTimings(Time::Milliseconds total, Time::Milliseconds loop) override
    {
      if (total)
      {
        Info = CreateTimedInfo(total, loop);
      }
    }

    Information::Ptr CaptureResult(const Parameters::Accessor& props)
    {
      if (Info)
      {
        return Information::Ptr(std::move(Info));
      }
      else
      {
        const auto duration = GetDefaultDuration(props);
        return CreateTimedInfo(duration, duration);
      }
    }

  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    Information::Ptr Info;
  };

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::VideoGameMusic::Parse(rawData, dataBuilder))
        {
          auto tune = MakePtr<LibVGM::Model>(&LibVGM::Create< ::VGMPlayer>, *container);
          // TODO: move to builder
          props.SetPlatform(DetectPlatform(*tune->Data));

          props.SetSource(*container);
          auto layout = MakePtr<Module::LibVGM::ChannelsLayout>(*tune);
          props.SetChannels(layout->GetChannelsNames());
          auto info = dataBuilder.CaptureResult(*properties);

          return MakePtr<LibVGM::Holder>(std::move(tune), std::move(info), std::move(layout), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        LibVGM::Dbg("Failed to create VGM: {}", e.what());
      }
      return {};
    }
  };
}  // namespace Module::VideoGameMusic

namespace Module::Sound98
{
  class DataBuilder : public Formats::Chiptune::Sound98::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetTimings(Time::Milliseconds total, Time::Milliseconds loop) override
    {
      Info = CreateTimedInfo(total, loop);
    }

    Module::Information::Ptr CaptureResult() const
    {
      return Info;
    }

  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    Module::Information::Ptr Info;
  };

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::Sound98::Parse(rawData, dataBuilder))
        {
          auto tune = MakePtr<LibVGM::Model>(&LibVGM::Create< ::S98Player>, *container);

          props.SetSource(*container);
          auto layout = MakePtr<Module::LibVGM::ChannelsLayout>(*tune);
          props.SetChannels(layout->GetChannelsNames());

          return MakePtr<LibVGM::Holder>(std::move(tune), dataBuilder.CaptureResult(), std::move(layout),
                                         std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        LibVGM::Dbg("Failed to create S98: {}", e.what());
      }
      return {};
    }
  };
}  // namespace Module::Sound98

namespace ZXTune
{
  void RegisterVGMPlugins(PlayerPluginsRegistrator& registrator)
  {
    {
      const auto ID = "VGM"_id;
      const uint_t CAPS = ZXTune::Capabilities::Module::Type::STREAM | ZXTune::Capabilities::Module::Device::MULTI;
      auto decoder = Formats::Chiptune::CreateVideoGameMusicDecoder();
      auto factory = MakePtr<Module::VideoGameMusic::Factory>();
      auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
    {
      const auto ID = "S98"_id;
      const uint_t CAPS = ZXTune::Capabilities::Module::Type::STREAM | ZXTune::Capabilities::Module::Device::MULTI;
      auto decoder = Formats::Chiptune::CreateSound98Decoder();
      auto factory = MakePtr<Module::Sound98::Factory>();
      auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
