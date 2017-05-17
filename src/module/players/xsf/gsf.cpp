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
#include "gsf.h"
#include "gsf_rom.h"
#include "xsf.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <binary/compression/zlib_container.h>
#include <debug/log.h>
#include <module/additional_files.h>
#include <module/players/analyzer.h>
#include <module/players/duration.h>
#include <module/players/fading.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//std includes
#include <map>
//3rdparty includes
#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba-util/vfs.h>

#undef min

#define FILE_TAG AD921CF6

namespace Module
{
namespace GSF
{
  const Debug::Stream Dbg("Module::GSFSupp");

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
      const auto romFile = VFileMemChunk(rom.Data.data(), rom.Data.size());
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
  
  class GbaEngine : public Module::Analyzer
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

    std::vector<ChannelState> GetState() const override
    {
      return std::vector<ChannelState>();
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
      , SoundParams(Sound::RenderParameters::Create(params))
      , Target(Module::CreateFadingReceiver(std::move(params), std::move(info), State, std::move(target)))
      , SamplesPerFrame()
      , Looped()
    {
      ApplyParameters();
    }

    TrackState::Ptr GetTrackState() const override
    {
      return State;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Engine;
    }

    bool RenderFrame() override
    {
      try
      {
        ApplyParameters();

        Target->ApplyData(Engine->Render(SamplesPerFrame));
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
    const TrackState::Ptr State;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    uint_t SamplesPerFrame;
    bool Looped;
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
    
    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties, Time::Seconds defaultDuration)
    {
      const auto period = Sound::GetFrameDuration(*properties);
      const decltype(period) duration = tune->Meta && tune->Meta->Duration.Get() ? tune->Meta->Duration : decltype(tune->Meta->Duration)(defaultDuration);
      const uint_t frames = duration.Get() / period.Get();
      const Information::Ptr info = CreateStreamInfo(frames);
      if (tune->Meta)
      {
        tune->Meta->Dump(*properties);
      }
      return MakePtr<Holder>(std::move(tune), info, std::move(properties));
    }
  private:
    const ModuleData::Ptr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  class ModuleDataBuilder
  {
  public:
    void AddRom(Binary::Data::Ptr packedSection)
    {
      Require(!!packedSection);
      if (!Rom)
      {
        Rom = MakeRWPtr<GbaRom>();
      }
      const auto unpackedSection = Binary::Compression::Zlib::CreateDeferredDecompressContainer(std::move(packedSection));
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
  
  class XsfView
  {
  public:
    explicit XsfView(const XSF::File& file)
      : File(file)
    {
    }
    
    bool IsSingleTrack() const
    {
      return File.Dependencies.empty();
    }
    
    bool IsMultiTrack() const
    {
      return !File.Dependencies.empty();
    }
    
    ModuleData::Ptr CreateModuleData() const
    {
      Require(File.Dependencies.empty());
      ModuleDataBuilder builder;
      if (File.Meta)
      {
        builder.AddMeta(*File.Meta);
      }
      builder.AddRom(File.PackedProgramSection);
      //don't know anything about reserved section state
      return builder.CaptureResult();
    }
  private:
    const XSF::File& File;
  };
 
  class MultiFileHolder : public Module::Holder
                        , public Module::AdditionalFiles
  {
  public:
    MultiFileHolder(XSF::File head, Parameters::Container::Ptr properties, Time::Seconds defaultDuration)
      : DefaultDuration(defaultDuration)
      , Properties(std::move(properties))
      , Head(std::move(head))
    {
      LoadDependenciesFrom(Head);
      Head.CloneData();
    }
    
    Module::Information::Ptr GetModuleInformation() const override
    {
      return GetDelegate().GetModuleInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return GetDelegate().GetModuleProperties();
    }

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return GetDelegate().CreateRenderer(std::move(params), std::move(target));
    }
    
    Strings::Array Enumerate() const override
    {
      Strings::Array result;
      for (const auto& dep : Dependencies)
      {
        if (0 == dep.second.Version)
        {
          result.push_back(dep.first);
        }
      }
      return result;
    }
    
    void Resolve(const String& name, Binary::Container::Ptr data) override
    {
      XSF::File file;
      if (XSF::Parse(name, *data, file))
      {
        Dbg("Resolving dependency '%1%'", name);
        LoadDependenciesFrom(file);
        file.CloneData();
        const auto it = Dependencies.find(name);
        Require(it != Dependencies.end() && 0 == it->second.Version);
        it->second = std::move(file);
      }
    }
  private:
    void LoadDependenciesFrom(const XSF::File& file)
    {
      Require(Head.Version == file.Version);
      for (const auto& dep : file.Dependencies)
      {
        Require(!dep.empty());
        Require(Dependencies.emplace(dep, XSF::File()).second);
        Dbg("Found unresolved dependency '%1%'", dep);
      }
    }
    
    const Module::Holder& GetDelegate() const
    {
      if (!Delegate)
      {
        Require(!Dependencies.empty());
        auto mergedData = MergeDependencies();
        FillStrings();
        Delegate = GSF::Holder::Create(std::move(mergedData), std::move(Properties), DefaultDuration);
        Dependencies.clear();
        Head = XSF::File();
      }
      return *Delegate;
    }
    
    ModuleData::Ptr MergeDependencies() const
    {
      ModuleDataBuilder builder;
      MergeRom(Head, builder);
      MergeMeta(Head, builder);
      return builder.CaptureResult();
    }
    
    /* https://bitbucket.org/zxtune/zxtune/wiki/GSFFormat
    
    Look at the official psf specs for lib loading order.  Multiple libs are
    now supported.

    */
    void MergeRom(const XSF::File& data, ModuleDataBuilder& dst) const
    {
      auto it = data.Dependencies.begin();
      const auto lim = data.Dependencies.end();
      if (it != lim)
      {
        MergeRom(GetDependency(*it), dst);
      }
      dst.AddRom(data.PackedProgramSection);
      if (it != lim)
      {
        for (++it; it != lim; ++it)
        {
          MergeRom(GetDependency(*it), dst);
        }
      }
    }
    
    void MergeMeta(const XSF::File& data, ModuleDataBuilder& dst) const
    {
      for (const auto& dep : data.Dependencies)
      {
        MergeMeta(GetDependency(dep), dst);
      }
      if (data.Meta)
      {
        dst.AddMeta(*data.Meta);
      }
    }
    
    const XSF::File& GetDependency(const String& name) const
    {
      const auto it = Dependencies.find(name);
      if (it == Dependencies.end() || 0 == it->second.Version)
      {
        Dbg("GSF: unresolved '%1%'", name);
        throw MakeFormattedError(THIS_LINE, "Unresolved dependency '%1%'", name);
      }
      Dbg("GSF: apply '%1%'", name);
      return it->second;
    }
    
    void FillStrings() const
    {
      Strings::Array linear;
      linear.reserve(Dependencies.size());
      for (const auto& dep : Dependencies)
      {
        linear.push_back(dep.first);
      }
      PropertiesHelper(*Properties).SetStrings(linear);
    }
  private:
    const Time::Seconds DefaultDuration;
    mutable Parameters::Container::Ptr Properties;
    mutable XSF::File Head;
    mutable std::map<String, XSF::File> Dependencies;
    
    mutable Holder::Ptr Delegate;
  };
  
  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        Dbg("Try to parse GSF");
        XSF::File file;
        if (const auto source = XSF::Parse(rawData, file))
        {
          PropertiesHelper props(*properties);
          props.SetSource(*source);

          const XsfView xsf(file);
          if (xsf.IsSingleTrack())
          {
            Dbg("Singlefile GSF");
            auto tune = xsf.CreateModuleData();
            return Holder::Create(std::move(tune), std::move(properties), GetDuration(params));
          }
          else if (xsf.IsMultiTrack())
          {
            Dbg("Multifile GSF");
            return MakePtr<MultiFileHolder>(std::move(file), std::move(properties), GetDuration(params));
          }
          else
          {
            Dbg("Invalid GSF");
          }
        }
      }
      catch (const std::exception&)
      {
        Dbg("Failed to parse GSF");
      }
      return Module::Holder::Ptr();
    }
  };
  
  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}
