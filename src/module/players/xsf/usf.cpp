/**
*
* @file
*
* @brief  USF chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "usf.h"
#include "xsf.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <binary/container_factories.h>
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
#include <sound/resampler.h>
#include <sound/sound_parameters.h>
//std includes
#include <list>
#include <map>
//3rdparty includes
#include <3rdparty/lazyusf2/usf/usf.h>

#define FILE_TAG AA708C36

namespace Module
{
namespace USF
{
  const Debug::Stream Dbg("Module::USFSupp");

  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;
    
    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;
    
    std::list<Dump> Sections;
    XSF::MetaInformation::Ptr Meta;
    
    uint_t GetRefreshRate() const
    {
      if (Meta && Meta->RefreshRate)
      {
        return Meta->RefreshRate;
      }
      else
      {
        return 60;//NTSC by default
      }
    }
  };
  
  class UsfHolder
  {
  public:
    UsfHolder()
      : Data(new uint8_t[::usf_get_state_size()])
    {
      ::usf_clear(GetRaw());
    }
    
    UsfHolder(const UsfHolder&) = delete;
    UsfHolder& operator = (const UsfHolder&) = delete;
    
    ~UsfHolder()
    {
      ::usf_shutdown(GetRaw());
    }
    
    void* GetRaw()
    {
      return Data.get();
    }
    
    /*usf_state* GetInternal()
    {
      return safe_ptr_cast<usf_state*>(Data.get());
    }*/
  private:
    std::unique_ptr<uint8_t[]> Data;
  };
  
  class USFEngine : public Module::Analyzer
  {
  public:
    using Ptr = std::shared_ptr<USFEngine>;
  
    explicit USFEngine(const ModuleData& data)
      : Emu()
    {
      SetupSections(data.Sections);
      if (data.Meta)
      {
        SetupEnvironment(*data.Meta);
      }
      DetectSoundFrequency();
      Dbg("Used sound frequency is %1%", SoundFrequency);
    }
    
    void Reset()
    {
      ::usf_restart(Emu.GetRaw());
    }

    uint_t GetSoundFrequency() const
    {
      return SoundFrequency;
    }
    
    Sound::Chunk Render(uint_t samples)
    {
      Sound::Chunk result(samples);
      for (uint32_t doneSamples = 0; doneSamples < samples; )
      {
        const auto toRender = std::min<uint32_t>(samples - doneSamples, 1024);
        const auto dst = safe_ptr_cast<short int*>(&result[doneSamples]);
        const auto res = ::usf_render(Emu.GetRaw(), dst, toRender, nullptr);
        Require(res == nullptr);
        doneSamples += toRender;
      }
      return result;
    }
    
    void Skip(uint_t samples)
    {
      for (uint32_t skippedSamples = 0; skippedSamples < samples; )
      {
        const auto toSkip = std::min<uint32_t>(samples - skippedSamples, 1024);
        const auto res = ::usf_render(Emu.GetRaw(), nullptr, toSkip, nullptr);
        Require(res == nullptr);
        skippedSamples += toSkip;
      }
    }

    std::vector<ChannelState> GetState() const override
    {
      return std::vector<ChannelState>();
    }
  private:
    void SetupSections(const std::list<Dump>& sections)
    {
      for (const auto& blob : sections)
      {
        Require(-1 != ::usf_upload_section(Emu.GetRaw(), blob.data(), blob.size()));
      }
    }
    
    void SetupEnvironment(const XSF::MetaInformation& meta)
    {
      for (const auto& tag : meta.Tags)
      {
        if (tag.first == "_enablecompare")
        {
          ::usf_set_compare(Emu.GetRaw(), true);
        }
        else if (tag.first == "_enablefifofull")
        {
          ::usf_set_fifo_full(Emu.GetRaw(), true);
        }
      }
      ::usf_set_hle_audio(Emu.GetRaw(), true);
    }
    
    void DetectSoundFrequency()
    {
      int32_t freq = 0;
      Require(nullptr == ::usf_render(Emu.GetRaw(), nullptr, 0, &freq));
      Reset();
      SoundFrequency = freq;
    }
  private:
    UsfHolder Emu;
    uint_t SoundFrequency = 0;
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(const ModuleData& data, Information::Ptr info, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Engine(MakePtr<USFEngine>(data))
      , Iterator(Module::CreateStreamStateIterator(info))
      , State(Iterator->GetStateObserver())
      , SoundParams(Sound::RenderParameters::Create(params))
      , Target(Module::CreateFadingReceiver(std::move(params), std::move(info), State, std::move(target)))
      , Looped()
    {
      SamplesPerFrame = Engine->GetSoundFrequency() / data.GetRefreshRate();
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
        Looped = SoundParams->Looped();
        Resampler = Sound::CreateResampler(Engine->GetSoundFrequency(), SoundParams->SoundFreq(), Target);
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
    const USFEngine::Ptr Engine;
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    uint_t SamplesPerFrame;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
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
      return MakePtr<Renderer>(*Tune, Info, std::move(target), std::move(params));
    }
    
    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties, Time::Seconds defaultDuration)
    {
      const auto period = Time::GetPeriodForFrequency<Time::Milliseconds>(tune->GetRefreshRate());
      const auto duration = tune->Meta && tune->Meta->Duration.Get() ? tune->Meta->Duration : Time::Milliseconds(defaultDuration);
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
  
  Dump CreateDump(const Binary::Data& data)
  {
    const auto start = static_cast<const uint8_t*>(data.Start());
    const auto size = data.Size();
    return Dump(start, start + size);
  }
  
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
      const ModuleData::RWPtr result = MakeRWPtr<ModuleData>();
      result->Meta = File.Meta;
      Require(!File.ProgramSection);
      Require(!!File.ReservedSection);
      result->Sections.emplace_back(CreateDump(*File.ReservedSection));
      return result;
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
        Delegate = USF::Holder::Create(std::move(mergedData), std::move(Properties), DefaultDuration);
        Dependencies.clear();
        Head = XSF::File();
      }
      return *Delegate;
    }
    
    class ModuleDataBuilder
    {
    public:
      void AddSection(const Binary::Data& data)
      {
        Sections.emplace_back(CreateDump(data));
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
        res->Sections = std::move(Sections);
        res->Meta = std::move(Meta);
        return res;
      }
    private:
      std::list<Dump> Sections;
      XSF::MetaInformation::RWPtr Meta;
    };
    
    ModuleData::Ptr MergeDependencies() const
    {
      ModuleDataBuilder builder;
      MergeSections(Head, builder);
      return builder.CaptureResult();
    }
    
    void MergeSections(const XSF::File& data, ModuleDataBuilder& dst) const
    {
      Require(!!data.ReservedSection);
      auto it = data.Dependencies.begin();
      const auto lim = data.Dependencies.end();
      if (it != lim)
      {
        MergeSections(GetDependency(*it), dst);
      }
      dst.AddSection(*data.ReservedSection);
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
        Dbg("USF: unresolved '%1%'", name);
        throw MakeFormattedError(THIS_LINE, "Unresolved dependency '%1%'", name);
      }
      Dbg("USF: apply '%1%'", name);
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
        Dbg("Try to parse USF");
        XSF::File file;
        if (const auto source = XSF::Parse(rawData, file))
        {
          PropertiesHelper props(*properties);
          props.SetSource(*source);

          const XsfView xsf(file);
          if (xsf.IsSingleTrack())
          {
            Dbg("Singlefile USF");
            auto tune = xsf.CreateModuleData();
            return Holder::Create(std::move(tune), std::move(properties), GetDuration(params));
          }
          else if (xsf.IsMultiTrack())
          {
            Dbg("Multifile USF");
            return MakePtr<MultiFileHolder>(std::move(file), std::move(properties), GetDuration(params));
          }
          else
          {
            Dbg("Invalid USF");
          }
        }
      }
      catch (const std::exception&)
      {
        Dbg("Failed to parse USF");
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
