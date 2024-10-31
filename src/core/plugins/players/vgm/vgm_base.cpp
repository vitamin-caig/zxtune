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
#include <formats/chiptune/multidevice/sound98.h>
#include <formats/chiptune/multidevice/videogamemusic.h>
#include <module/players/duration.h>
#include <module/players/platforms.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>

#include <binary/container_factories.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <module/attributes.h>

#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>

#include <3rdparty/vgm/player/s98player.hpp>
#include <3rdparty/vgm/player/vgmplayer.hpp>
#include <3rdparty/vgm/utils/DataLoader.h>

#include <map>

namespace Module::LibVGM
{
  const Debug::Stream Dbg("Core::VGMSupp");

  using PlayerPtr = std::unique_ptr< ::PlayerBase>;

  using PlayerCreator = PlayerPtr (*)();

  template<class PlayerType>
  PlayerPtr Create()
  {
    return PlayerPtr(new PlayerType());
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
                                                      &Close,     &Tell, &Length, &Eof};
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

  const Time::Milliseconds FRAME_DURATION(20);

  class VGMEngine : public State
  {
  public:
    using RWPtr = std::shared_ptr<VGMEngine>;

    VGMEngine(Model::Ptr tune, const Module::Information& info, uint_t samplerate)
      : Tune(std::move(tune))
      , Loader(*Tune->Data)
      , Delegate(Tune->CreatePlayer())
    {
      Require(0 == Delegate->LoadFile(Loader.Get()));
      Require(0 == Delegate->SetSampleRate(samplerate));
      Delegate->Start();
      TotalTicks = ToTicks(info.Duration());
      LoopTicks = ToTicks(info.LoopDuration());
    }

    Time::AtMillisecond At() const override
    {
      auto ticks = Delegate->GetCurPos(PLAYPOS_TICK);
      if (ticks >= TotalTicks)
      {
        ticks = LoopTicks != 0 ? (TotalTicks - LoopTicks) + (ticks - TotalTicks) % LoopTicks : ticks % TotalTicks;
      }
      return Time::AtMillisecond() + Time::Seconds(Delegate->Tick2Second(ticks));
    }

    Time::Milliseconds Total() const override
    {
      const auto ticks = Delegate->GetCurPos(PLAYPOS_TICK);
      return Time::Seconds(Delegate->Tick2Second(ticks));
    }

    uint_t LoopCount() const override
    {
      // Some of the tracks specifies LoopTicks == 0 with proper looping
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
      Buffer.resize(samples);
      std::memset(Buffer.data(), 0, samples * sizeof(Buffer.front()));
      const auto outSamples = Delegate->Render(samples, Buffer.data());
      CheckForWholeLoop();
      return ConvertBuffer(outSamples);
    }

    void Seek(Time::AtMillisecond request)
    {
      const auto samples = ToSample(request);
      Require(0 == Delegate->Seek(PLAYPOS_SAMPLE, samples));
      WholeLoopCount = 0;
    }

  private:
    template<class Duration>
    uint_t ToTicks(Duration dur) const
    {
      const auto sample = ToSample(dur);
      return Delegate->Sample2Tick(sample);
    }

    template<class Item>
    uint_t ToSample(Item it) const
    {
      return uint_t(uint64_t(it.Get()) * Delegate->GetSampleRate() / Item::PER_SECOND);
    }

    void CheckForWholeLoop()
    {
      if (0 != (Delegate->GetState() & PLAYSTATE_END))
      {
        ++WholeLoopCount;
        Require(0 == Delegate->Seek(PLAYPOS_TICK, 0));
      }
    }

    Sound::Chunk ConvertBuffer(uint_t samples) const
    {
      Sound::Chunk result(samples);
      std::transform(Buffer.data(), Buffer.data() + samples, result.data(), &ConvertSample);
      return result;
    }

    static Sound::Sample ConvertSample(WAVE_32BS data)
    {
      return {Convert(data.L), Convert(data.R)};
    }

    static Sound::Sample::Type Convert(DEV_SMPL in)
    {
      return Math::Clamp<Sound::Sample::WideType>(in >> 8, Sound::Sample::MIN, Sound::Sample::MAX);
    }

  private:
    const Model::Ptr Tune;
    LoaderAdapter Loader;
    std::unique_ptr< ::PlayerBase> Delegate;
    uint_t TotalTicks = 0;
    uint_t LoopTicks = 0;
    uint_t WholeLoopCount = 0;
    std::vector<WAVE_32BS> Buffer;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr tune, const Module::Information& info, uint_t samplerate)
      : Engine(MakeRWPtr<VGMEngine>(std::move(tune), info, samplerate))
    {}

    State::Ptr GetState() const override
    {
      return Engine;
    }

    Sound::Chunk Render() override
    {
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
    const VGMEngine::RWPtr Engine;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr tune, Module::Information::Ptr info, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Info(std::move(info))
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

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      try
      {
        return MakePtr<Renderer>(Tune, *Info, samplerate);
      }
      catch (const std::exception& e)
      {
        throw Error(THIS_LINE, e.what());
      }
    }

  private:
    const Model::Ptr Tune;
    const Module::Information::Ptr Info;
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
          auto info = dataBuilder.CaptureResult(*properties);

          return MakePtr<LibVGM::Holder>(std::move(tune), std::move(info), std::move(properties));
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

          return MakePtr<LibVGM::Holder>(std::move(tune), dataBuilder.CaptureResult(), std::move(properties));
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
