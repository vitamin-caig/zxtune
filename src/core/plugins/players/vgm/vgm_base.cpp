/**
*
* @file
*
* @brief  libvgm-based formats support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/vgm/videogamemusic.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/multidevice/videogamemusic.h>
#include <math/numeric.h>
#include <module/attributes.h>
#include <module/players/analyzer.h>
#include <module/players/duration.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//3rdparty includes
#include <3rdparty/vgm/player/vgmplayer.hpp>
#include <3rdparty/vgm/utils/DataLoader.h>
//std includes
#include <map>
//text includes
#include <module/text/platforms.h>

#define FILE_TAG 975CF2F9

namespace Module
{
namespace LibVGM
{
  const Debug::Stream Dbg("Core::VGMSupp");

  using PlayerPtr = std::unique_ptr< ::PlayerBase>;

  typedef PlayerPtr (*PlayerCreator)();

  template<class PlayerType>
  PlayerPtr Create()
  {
    return PlayerPtr(new PlayerType());
  }

  struct Model
  {
    using Ptr = std::shared_ptr<Model>;

    Model(PlayerCreator create, Dump data)
      : CreatePlayer(create)
      , Data(std::move(data))
    {
    }

    PlayerCreator CreatePlayer;
    Dump Data;
  };

  class LoaderAdapter
  {
  public:
    explicit LoaderAdapter(Binary::View raw)
      : Raw(raw)
    {
      std::memset(&Delegate, 0, sizeof(Delegate));
      static const DATA_LOADER_CALLBACKS CALLBACKS =
      {
        0xdeadbeef,
        "",
        &Open, &Read, nullptr, &Close, &Tell, &Length, &Eof
      };
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

    explicit VGMEngine(Model::Ptr tune)
      : Tune(std::move(tune))
      , Loader(Tune->Data)
      , Delegate(Tune->CreatePlayer())
    {
      Require(0 == Delegate->LoadFile(Loader.Get()));
      LoopTicks = Delegate->GetLoopTicks();
    }

    uint_t Frame() const override
    {
      auto ticks = Delegate->GetCurPos(PLAYPOS_TICK);
      const auto totalTicks = Delegate->GetTotalTicks();
      if (ticks >= totalTicks)
      {
        ticks = LoopTicks != 0
          ? (totalTicks - LoopTicks) + (ticks - totalTicks) % LoopTicks
          : ticks % totalTicks;
      }
      const auto seconds = Delegate->Tick2Second(ticks);
      return FRAME_DURATION.PER_SECOND * seconds / FRAME_DURATION.Get();
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

    void SetSoundFreq(uint_t freq)
    {
      Delegate->Stop();
      Delegate->SetSampleRate(freq);
      Delegate->Start();
    }

    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");

      Buffer.resize(samples);
      std::memset(Buffer.data(), 0, samples * sizeof(Buffer.front()));
      const auto outSamples = Delegate->Render(samples, Buffer.data());
      CheckForWholeLoop();
      return ConvertBuffer(outSamples);
    }

    void Seek(uint_t samples)
    {
      Require(0 == Delegate->Seek(PLAYPOS_SAMPLE, samples));
      WholeLoopCount = 0;
    }
  private:
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
      return Sound::Sample(Convert(data.L), Convert(data.R));
    }

    static Sound::Sample::Type Convert(DEV_SMPL in)
    {
      return in >> 8;
    }
  private:
    const Model::Ptr Tune;
    LoaderAdapter Loader;
    std::unique_ptr< ::PlayerBase> Delegate;
    uint_t LoopTicks;
    uint_t WholeLoopCount = 0;
    std::vector<WAVE_32BS> Buffer;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr tune, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Engine(MakeRWPtr<VGMEngine>(std::move(tune)))
      , Analyzer(CreateSoundAnalyzer())
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
      , Target(std::move(target))
      , Looped()
    {
      ApplyParameters();
    }

    State::Ptr GetState() const override
    {
      return Engine;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Analyzer;
    }

    bool RenderFrame() override
    {
      try
      {
        ApplyParameters();

        auto data = Engine->Render(SoundParams->SamplesPerFrame());
        Analyzer->AddSoundData(data);
        Target->ApplyData(std::move(data));

        const auto loops = Engine->LoopCount();
        return loops == 0 || Looped(loops);
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      try
      {
        SoundParams.Reset();
        Engine->Reset();
        Looped = {};
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

    void SetPosition(uint_t frame) override
    {
      try
      {
        Engine->Seek(frame * SoundParams->SamplesPerFrame());
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
        Engine->SetSoundFreq(SoundParams->SoundFreq());
      }
    }

  private:
    const VGMEngine::RWPtr Engine;
    const SoundAnalyzer::Ptr Analyzer;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    Sound::LoopParameters Looped;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr tune, Information::Ptr info, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Info(std::move(info))
      , Properties(std::move(props))
    {
    }

    Module::Information::Ptr GetModuleInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      try
      {
        return MakePtr<Renderer>(Tune, target, params);
      }
      catch (const std::exception& e)
      {
        throw Error(THIS_LINE, e.what());
      }
    }
  private:
    const Model::Ptr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
} //namespace LibVGM

namespace VideoGameMusic
{
  class DataBuilder : public Formats::Chiptune::VideoGameMusic::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
    {
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetTimings(Time::Milliseconds total, Time::Milliseconds loop) override
    {
      Info = CreateStreamInfo(total.Get() / LibVGM::FRAME_DURATION.Get(), loop.Get() / LibVGM::FRAME_DURATION.Get());
    }

    Information::Ptr GetInfo() const
    {
      return Info;
    }
  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    Information::Ptr Info;
  };
  
  Dump CreateData(Binary::View data)
  {
    return Dump(static_cast<const uint8_t*>(data.Start()), static_cast<const uint8_t*>(data.Start()) + data.Size());
  }

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::VideoGameMusic::Parse(rawData, dataBuilder))
        {
          auto data = CreateData(*container);
          //TODO: move to builder
          props.SetPlatform(DetectPlatform(data));
          auto tune = MakePtr<LibVGM::Model>(&LibVGM::Create< ::VGMPlayer>, std::move(data));
        
          props.SetSource(*container);
        
          return MakePtr<LibVGM::Holder>(std::move(tune), dataBuilder.GetInfo(), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        LibVGM::Dbg("Failed to create VGM: %2%", e.what());
      }
      return Module::Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterVGMPlugins(PlayerPluginsRegistrator& registrator)
  {
    {
      const Char ID[] = {'V', 'G', 'M'};
      const uint_t CAPS = ZXTune::Capabilities::Module::Type::STREAM | ZXTune::Capabilities::Module::Device::MULTI;
      auto decoder = Formats::Chiptune::CreateVideoGameMusicDecoder();
      auto factory = MakePtr<Module::VideoGameMusic::Factory>();
      auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(plugin);
    }
  }
}

#undef FILE_TAG
