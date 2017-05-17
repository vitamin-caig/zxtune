/**
*
* @file
*
* @brief  2SF chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "2sf.h"
#include "memory_region.h"
#include "xsf.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
#include <binary/compression/zlib_container.h>
#include <debug/log.h>
#include <devices/details/analysis_map.h>
#include <formats/chiptune/emulation/nintendodssoundformat.h>
#include <math/bitops.h>
#include <module/additional_files.h>
#include <module/players/analyzer.h>
#include <module/players/duration.h>
#include <module/players/fading.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/resampler.h>
#include <sound/sound_parameters.h>
//std includes
#include <list>
#include <map>
//3rdparty includes
#include <3rdparty/vio2sf/desmume/SPU.h>
#include <3rdparty/vio2sf/desmume/state.h>

#define FILE_TAG 6934F890

namespace Module
{
namespace TwoSF
{
  const Debug::Stream Dbg("Module::2SFSupp");
  
  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;
    
    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;
    
    std::list<Binary::Container::Ptr> PackedProgramSections;
    std::list<Binary::Container::Ptr> ReservedSections;

    XSF::MetaInformation::Ptr Meta;
  };
  
  class AnalysisMap
  {
  public:
    AnalysisMap()
    {
      const auto CLOCKRATE = 33513982 / 2;
      const auto C3_RATE = 130.81f;
      const auto C3_SAMPLERATE = 8000;
      Delegate.SetClockRate(CLOCKRATE * C3_RATE / C3_SAMPLERATE);
    }
    
    uint_t GetBand(uint_t timer) const
    {
      return Delegate.GetBandByPeriod(0x10000 - timer);
    }
  private:
    Devices::Details::AnalysisMap Delegate;
  };
  
  class DSEngine : public Module::Analyzer
  {
  public:
    using Ptr = std::shared_ptr<DSEngine>;
  
    enum
    {
      SAMPLERATE = 44100
    };
    
    explicit DSEngine(const ModuleData& data)
    {
      Require(0 == ::state_init(&State));
      if (data.Meta)
      {
        SetupEnvironment(*data.Meta);
      }
      if (!data.PackedProgramSections.empty())
      {
        SetupRom(data.PackedProgramSections);
      }
      if (!data.ReservedSections.empty())
      {
        SetupState(data.ReservedSections);
      }
    }
    
    ~DSEngine() override
    {
      ::state_deinit(&State);
    }
    
    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Invalid sound channels count");
      static_assert(Sound::Sample::MID == 0, "Invalid sound sample type");
      static_assert(Sound::Sample::MAX == 32767 && Sound::Sample::MIN == -32768, "Invalid sound sample type");

      Sound::Chunk res(samples);
      ::state_render(&State, safe_ptr_cast<s16*>(res.data()), samples);
      return std::move(res);
    }
    
    void Skip(uint_t samples)
    {
      const auto prevInterpolation = State.dwInterpolation;
      State.dwInterpolation = 0;
      {
        const auto BUFFER_SIZE = 1024;
        s16 dummy[BUFFER_SIZE * Sound::Sample::CHANNELS];
        while (samples)
        {
          const auto toRender = std::min<uint_t>(samples, BUFFER_SIZE);
          ::state_render(&State, dummy, toRender);
          samples -= toRender;
        }
      }
      State.dwInterpolation = prevInterpolation;
    }

    std::vector<ChannelState> GetState() const override
    {
      static const AnalysisMap ANALYSIS;
      std::vector<ChannelState> result;
      result.reserve(16);
      for (const auto& in : State.SPU_core->channels)
      {
        if (in.status == CHANSTAT_STOPPED)
        {
          continue;
        }
        ChannelState out;
        out.Level = ::spumuldiv7(100, in.vol) >> in.datashift;
        if (out.Level)
        {
          out.Band = ANALYSIS.GetBand(in.timer);
          result.push_back(out);
        }
      }
      return result;
    }
  private:
    void SetupEnvironment(const XSF::MetaInformation& meta)
    {
      int clockDown = 0;
      State.dwChannelMute = 0;
      State.initial_frames = -1;
      //TODO: interpolation
      for (const auto& tag : meta.Tags)
      {
        if (tag.first == "_clockdown")
        {
          clockDown = std::atoi(tag.second.c_str());
        }
        else if (auto target = FindTagTarget(tag.first))
        {
          *target = std::atoi(tag.second.c_str());
        }
      }
      if (!State.arm7_clockdown_level && clockDown)
      {
        State.arm7_clockdown_level = clockDown;
      }
      if (!State.arm9_clockdown_level && clockDown)
      {
        State.arm9_clockdown_level = clockDown;
      }
    }
    
    int* FindTagTarget(const String& name)
    {
      if (name == "_frames")
      {
        return &State.initial_frames;
      }
      else if (name == "_vio2sf_sync_type")
      {
        return &State.sync_type;
      }
      else if (name == "_vio2sf_arm9_clockdown_level")
      {
        return &State.arm9_clockdown_level;
      }
      else if (name == "_vio2sf_arm7_clockdown_level")
      {
        return &State.arm7_clockdown_level;
      }
      else
      {
        return nullptr;
      }
    }
    
    void SetupRom(const std::list<Binary::Container::Ptr>& blocks)
    {
      ChunkBuilder builder;
      for (const auto& block : blocks)
      {
        const auto unpacked = Binary::Compression::Zlib::CreateDeferredDecompressContainer(block);
        Formats::Chiptune::NintendoDSSoundFormat::ParseRom(*unpacked, builder);
      }
      //possibly, emulation writes to ROM are, so copy it
      Rom = builder.CaptureResult();
      //required power of 2 size
      const auto alignedRomSize = uint32_t(1) << Math::Log2(Rom.Data.size());
      Rom.Data.resize(alignedRomSize);
      ::state_setrom(&State, Rom.Data.data(), alignedRomSize, false/*coverage*/);
    }
    
    void SetupState(const std::list<Binary::Container::Ptr>& blocks)
    {
      ChunkBuilder builder;
      for (const auto& block : blocks)
      {
        Formats::Chiptune::NintendoDSSoundFormat::ParseState(*block, builder);
      }
      const auto& state = builder.CaptureResult();
      ::state_loadstate(&State, state.Data.data(), state.Data.size());
    }
  private:
    class ChunkBuilder : public Formats::Chiptune::NintendoDSSoundFormat::Builder
    {
    public:
      void SetChunk(uint32_t offset, const Binary::Data& content) override
      {
        Result.Update(offset, content.Start(), content.Size());
      }
      
      MemoryRegion CaptureResult()
      {
        return std::move(Result);
      }
    private:
      MemoryRegion Result;
    };
  private:
    NDS_state State;
    MemoryRegion Rom;
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr data, Information::Ptr info, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Data(std::move(data))
      , Iterator(Module::CreateStreamStateIterator(info))
      , State(Iterator->GetStateObserver())
      , SoundParams(Sound::RenderParameters::Create(params))
      , Target(Module::CreateFadingReceiver(std::move(params), std::move(info), State, std::move(target)))
      , Engine(MakePtr<DSEngine>(*Data))
      , Looped()
    {
      const auto frameDuration = SoundParams->FrameDuration();
      SamplesPerFrame = frameDuration.Get() * DSEngine::SAMPLERATE / frameDuration.PER_SECOND;
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

        Resampler->ApplyData(Engine->Render(SamplesPerFrame));
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
      Engine = MakePtr<DSEngine>(*Data);
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
        Looped = SoundParams->Looped();
        Resampler = Sound::CreateResampler(DSEngine::SAMPLERATE, SoundParams->SoundFreq(), Target);
      }
    }

    void SeekTune(uint_t frame)
    {
      uint_t current = State->Frame();
      if (frame < current)
      {
        Engine = MakePtr<DSEngine>(*Data);
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        Engine->Skip(delta * SamplesPerFrame);
      }
    }
  private:
    const ModuleData::Ptr Data;
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    uint_t SamplesPerFrame;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    DSEngine::Ptr Engine;
    Sound::Receiver::Ptr Resampler;
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
      return MakePtr<Renderer>(Tune, Info, std::move(target), std::move(params));
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
    ModuleDataBuilder()
      : Result(MakeRWPtr<ModuleData>())
    {
    }
    
    void AddProgramSection(Binary::Container::Ptr packedSection)
    {
      Require(!!packedSection);
      Result->PackedProgramSections.push_back(std::move(packedSection));
    }
    
    void AddReservedSection(Binary::Container::Ptr reservedSection)
    {
      Require(!!reservedSection);
      Result->ReservedSections.push_back(std::move(reservedSection));
    }
    
    void AddMeta(const XSF::MetaInformation& meta)
    {
      if (!Meta)
      {
        Result->Meta = Meta = MakeRWPtr<XSF::MetaInformation>(meta);
      }
      else
      {
        Meta->Merge(meta);
      }
    }
    
    ModuleData::Ptr CaptureResult()
    {
      return Result;
    }
  private:
    const ModuleData::RWPtr Result;
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
      if (File.PackedProgramSection)
      {
        builder.AddProgramSection(Binary::CreateContainer(File.PackedProgramSection->Start(), File.PackedProgramSection->Size()));
      }
      if (File.ReservedSection)
      {
        builder.AddReservedSection(Binary::CreateContainer(File.ReservedSection->Start(), File.ReservedSection->Size()));
      }
      if (File.Meta)
      {
        builder.AddMeta(*File.Meta);
      }
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
        const auto it = Dependencies.find(name);
        Require(it != Dependencies.end() && 0 == it->second.Version);
        LoadDependenciesFrom(file);
        file.CloneData();
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
        Delegate = TwoSF::Holder::Create(std::move(mergedData), std::move(Properties), DefaultDuration);
        Dependencies.clear();
        Head = XSF::File();
      }
      return *Delegate;
    }
    
    ModuleData::Ptr MergeDependencies() const
    {
      ModuleDataBuilder builder;
      MergeSections(Head, builder);
      MergeMeta(Head, builder);
      return builder.CaptureResult();
    }
    
    void MergeSections(const XSF::File& data, ModuleDataBuilder& dst) const
    {
      if (!data.Dependencies.empty())
      {
        MergeSections(GetDependency(data.Dependencies.front()), dst);
      }
      if (data.PackedProgramSection)
      {
        dst.AddProgramSection(data.PackedProgramSection);
      }
      if (data.ReservedSection)
      {
        dst.AddReservedSection(data.ReservedSection);
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
        Dbg("2SF: unresolved '%1%'", name);
        throw MakeFormattedError(THIS_LINE, "Unresolved dependency '%1%'", name);
      }
      Dbg("2SF: apply '%1%'", name);
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
        Dbg("Try to parse 2SF");
        XSF::File file;
        if (const auto source = XSF::Parse(rawData, file))
        {
          PropertiesHelper props(*properties);
          props.SetSource(*source);

          const XsfView xsf(file);
          if (xsf.IsSingleTrack())
          {
            Dbg("Singlefile 2SF");
            auto tune = xsf.CreateModuleData();
            return Holder::Create(std::move(tune), std::move(properties), GetDuration(params));
          }
          else if (xsf.IsMultiTrack())
          {
            Dbg("Multifile 2SF");
            return MakePtr<MultiFileHolder>(std::move(file), std::move(properties), GetDuration(params));
          }
          else
          {
            Dbg("Invalid 2SF");
          }
        }
      }
      catch (const std::exception&)
      {
        Dbg("Failed to parse 2SF");
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
