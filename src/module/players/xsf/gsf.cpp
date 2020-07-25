/**
*
* @file
*
* @brief  GSF chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/xsf/gsf.h"
#include "module/players/xsf/gsf_rom.h"
#include "module/players/xsf/xsf.h"
#include "module/players/xsf/xsf_factory.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/compression/zlib_container.h>
#include <debug/log.h>
#include <module/attributes.h>
#include <module/players/analyzer.h>
#include <module/players/fading.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//3rdparty includes
#include <3rdparty/mgba/defines.h>
#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba-util/vfs.h>
//text includes
#include <module/text/platforms.h>

#undef min

namespace Module
{
namespace GSF
{
  const Debug::Stream Dbg("Module::GSF");

  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;
    
    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;
    
    GbaRom::Ptr Rom;
    XSF::MetaInformation::Ptr Meta;
  };
  
  class AVStream : public mAVStream
  {
  public:
    enum
    {
      AUDIO_BUFFER_SIZE = 2048
    };
    
    AVStream()
    {
      std::memset(this, 0, sizeof(mAVStream));
    }
    
    void RenderSamples(uint_t count)
    {
      Require(SamplesToSkip == 0);
      Require(Result.empty());
      SamplesToRender = count;
      Result.resize(count);
      this->postAudioBuffer = &OnRenderComplete;
    }
    
    void SkipSamples(uint_t count)
    {
      Require(SamplesToRender == 0);
      Require(Result.empty());
      SamplesToSkip = count;
      this->postAudioBuffer = &OnSkipComplete;
    }
    
    bool IsReady() const
    {
      return SamplesToRender == 0 && SamplesToSkip == 0;
    }
    
    Sound::Chunk CaptureResult()
    {
      return std::move(Result);
    }

  private:
    static_assert(Sound::Sample::CHANNELS == 2, "Invalid sound channels count");
    static_assert(Sound::Sample::MID == 0, "Invalid sound sample type");
    static_assert(Sound::Sample::MAX == 32767 && Sound::Sample::MIN == -32768, "Invalid sound sample type");

    static void OnRenderComplete(struct mAVStream* in, blip_t* left, blip_t* right)
    {
      AVStream* const self = safe_ptr_cast<AVStream*>(in);
      const auto ready = self->Result.size() - self->SamplesToRender;
      const auto dst = safe_ptr_cast<int16_t*>(&self->Result[ready]);
      const auto done = std::min(blip_read_samples(left, dst, self->SamplesToRender, true), blip_read_samples(right, dst + 1, self->SamplesToRender, true));
      self->SamplesToRender -= done;
    }
    
    static void OnSkipComplete(struct mAVStream* in, blip_t* left, blip_t* right)
    {
      AVStream* const self = safe_ptr_cast<AVStream*>(in);
      int16_t dummy[AUDIO_BUFFER_SIZE];
      const auto toSkip = std::min<uint_t>(self->SamplesToSkip, AUDIO_BUFFER_SIZE);
      const auto done = std::min(blip_read_samples(left, dummy, toSkip, false), blip_read_samples(right, dummy, toSkip, false));
      self->SamplesToSkip -= done;
    }
  private:
    Sound::Chunk Result;
    uint_t SamplesToRender = 0;
    uint_t SamplesToSkip = 0;
  };
  
  class GbaCore
  {
  public:
    explicit GbaCore(const GbaRom& rom)
      : Core(GBACoreCreate())
    {
      Core->init(Core);
      mCoreInitConfig(Core, NULL);
      //core owns rom file memory, so copy it
      const auto romFile = VFileMemChunk(rom.Content.Data.data(), rom.Content.Data.size());
      Require(romFile != 0);
      Core->loadROM(Core, romFile);
      Reset();
    }
    
    ~GbaCore()
    {
      if (Core)
      {
        Core->deinit(Core);
        Core = nullptr;
      }
    }
    
    void InitSound(uint_t soundFreq, AVStream& stream)
    {
      Core->setAVStream(Core, &stream);
      Core->setAudioBufferSize(Core, AVStream::AUDIO_BUFFER_SIZE);
      const auto freq = Core->frequency(Core);
      for (uint_t chan = 0; chan < Sound::Sample::CHANNELS; ++chan)
      {
        blip_set_rates(Core->getAudioChannel(Core, chan), freq, soundFreq);
      }
      struct mCoreOptions opts;
      std::memset(&opts, 0, sizeof(opts));
      opts.useBios = false;
      opts.skipBios = true;
      opts.sampleRate = soundFreq;
      mCoreConfigLoadDefaults(&Core->config, &opts);
    }
    
    void Reset()
    {
      Core->reset(Core);
    }
    
    void RunFrame()
    {
      Core->runFrame(Core);
    }
  private:
    struct mCore* Core = nullptr;
  };
  
  class GbaEngine
  {
  public:
    using Ptr = std::shared_ptr<GbaEngine>;
  
    explicit GbaEngine(const ModuleData& data)
      : Core(*data.Rom)
    {
    }
    
    void SetParameters(const Sound::RenderParameters& params)
    {
      Core.InitSound(params.SoundFreq(), Stream);
    }
    
    void Reset()
    {
      Core.Reset();
    }

    Sound::Chunk Render(uint_t samples)
    {
      Stream.RenderSamples(samples);
      while (!Stream.IsReady())
      {
        Core.RunFrame();
      }
      return Stream.CaptureResult();
    }
    
    void Skip(uint_t samples)
    {
      Stream.SkipSamples(samples);
      while (!Stream.IsReady())
      {
        Core.RunFrame();
      }
    }
  private:
    GbaCore Core;
    AVStream Stream;
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(const ModuleData& data, Information::Ptr info, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Engine(MakePtr<GbaEngine>(data))
      , Iterator(Module::CreateStreamStateIterator(info))
      , State(Iterator->GetStateObserver())
      , Analyzer(CreateSoundAnalyzer())
      , SoundParams(Sound::RenderParameters::Create(params))
      , Target(Module::CreateFadingReceiver(std::move(params), std::move(info), State, std::move(target)))
      , SamplesPerFrame()
      , Looped()
    {
      ApplyParameters();
    }

    Module::State::Ptr GetState() const override
    {
      return State;
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
        auto data = Engine->Render(SamplesPerFrame);
        Analyzer->AddSoundData(data);
        Target->ApplyData(std::move(data));
        Iterator->NextFrame(Looped);
        return Iterator->IsValid();
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      SoundParams.Reset();
      Iterator->Reset();
      Engine->Reset();
      Looped = {};
    }

    void SetPosition(uint_t frame) override
    {
      SeekTune(frame);
      Module::SeekIterator(*Iterator, frame);
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        SamplesPerFrame = SoundParams->SamplesPerFrame();
        Looped = SoundParams->Looped();
        Engine->SetParameters(*SoundParams);
      }
    }

    void SeekTune(uint_t frame)
    {
      uint_t current = State->Frame();
      if (frame < current)
      {
        Engine->Reset();
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        Engine->Skip(delta * SamplesPerFrame);
      }
    }
  private:
    const GbaEngine::Ptr Engine;
    const StateIterator::Ptr Iterator;
    const Module::State::Ptr State;
    const Module::SoundAnalyzer::Ptr Analyzer;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    uint_t SamplesPerFrame;
    Sound::LoopParameters Looped;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModuleData::Ptr tune, Information::Ptr info, Parameters::Accessor::Ptr props)
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
      return MakePtr<Renderer>(*Tune, Info, std::move(target), std::move(params));
    }
    
    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties)
    {
      const auto period = Sound::GetFrameDuration(*properties);
      const auto duration = tune->Meta->Duration;
      const auto frames = duration.Divide<uint_t>(period);
      Information::Ptr info = CreateStreamInfo(frames);
      if (tune->Meta)
      {
        tune->Meta->Dump(*properties);
      }
      properties->SetValue(ATTR_PLATFORM, Platforms::GAME_BOY_ADVANCE);
      return MakePtr<Holder>(std::move(tune), std::move(info), std::move(properties));
    }
  private:
    const ModuleData::Ptr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  class ModuleDataBuilder
  {
  public:
    void AddRom(Binary::View packedSection)
    {
      Require(!!packedSection);
      if (!Rom)
      {
        Rom = MakeRWPtr<GbaRom>();
      }
      const auto unpackedSection = Binary::Compression::Zlib::Decompress(packedSection);
      GbaRom::Parse(*unpackedSection, *Rom);
    }

    void AddMeta(const XSF::MetaInformation& meta)
    {
      if (!Meta)
      {
        Meta = MakeRWPtr<XSF::MetaInformation>(meta);
      }
      else
      {
        Meta->Merge(meta);
      }
    }
    
    ModuleData::Ptr CaptureResult()
    {
      auto res = MakeRWPtr<ModuleData>();
      res->Rom = std::move(Rom);
      res->Meta = std::move(Meta);
      return res;
    }
  private:
    GbaRom::RWPtr Rom;
    XSF::MetaInformation::RWPtr Meta;
  };
  
  class Factory : public XSF::Factory
  {
  public:
    Holder::Ptr CreateSinglefileModule(const XSF::File& file, Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      if (file.Meta)
      {
        builder.AddMeta(*file.Meta);
      }
      builder.AddRom(*file.PackedProgramSection);
      //don't know anything about reserved section state
      return Holder::Create(builder.CaptureResult(), std::move(properties));
    }
    
    Holder::Ptr CreateMultifileModule(const XSF::File& file, const std::map<String, XSF::File>& additionalFiles, Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      MergeRom(file, additionalFiles, builder);
      MergeMeta(file, additionalFiles, builder);
      return Holder::Create(builder.CaptureResult(), std::move(properties));
    }
  private:
    /* https://bitbucket.org/zxtune/zxtune/wiki/GSFFormat
    
    Look at the official psf specs for lib loading order.  Multiple libs are
    now supported.

    */
    static const uint_t MAX_LEVEL = 10;

    static void MergeRom(const XSF::File& data, const std::map<String, XSF::File>& additionalFiles, ModuleDataBuilder& dst, uint_t level = 1)
    {
      auto it = data.Dependencies.begin();
      const auto lim = data.Dependencies.end();
      if (it != lim && level < MAX_LEVEL)
      {
        MergeRom(additionalFiles.at(*it), additionalFiles, dst, level + 1);
      }
      dst.AddRom(*data.PackedProgramSection);
      if (it != lim && level < MAX_LEVEL)
      {
        for (++it; it != lim; ++it)
        {
          MergeRom(additionalFiles.at(*it), additionalFiles, dst, level + 1);
        }
      }
    }
    
    static void MergeMeta(const XSF::File& data, const std::map<String, XSF::File>& additionalFiles, ModuleDataBuilder& dst, uint_t level = 1)
    {
      if (level < MAX_LEVEL)
      {
        for (const auto& dep : data.Dependencies)
        {
          MergeMeta(additionalFiles.at(dep), additionalFiles, dst, level + 1);
        }
      }
      if (data.Meta)
      {
        dst.AddMeta(*data.Meta);
      }
    }
  };
  
  Module::Factory::Ptr CreateFactory()
  {
    return XSF::CreateFactory(MakePtr<Factory>());
  }
}
}
