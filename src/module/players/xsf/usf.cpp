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
#include "module/players/xsf/usf.h"
#include "module/players/xsf/xsf.h"
#include "module/players/xsf/xsf_factory.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <module/attributes.h>
#include <module/players/analyzer.h>
#include <module/players/fading.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/resampler.h>
#include <sound/sound_parameters.h>
//std includes
#include <list>
//3rdparty includes
#include <3rdparty/lazyusf2/usf/usf.h>
//text includes
#include <module/text/platforms.h>

#define FILE_TAG 59F6FD73

namespace Module
{
namespace USF
{
  const Debug::Stream Dbg("Module::USF");
  
  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;
    
    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;
    
    std::list<Binary::Data::Ptr> Sections;
    XSF::MetaInformation::Ptr Meta;
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
  
  class USFEngine
  {
  public:
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
        if (const auto res = ::usf_render(Emu.GetRaw(), dst, toRender, nullptr))
        {
          throw MakeFormattedError(THIS_LINE, "USF: failed to render: %1%", res);
        }
        doneSamples += toRender;
      }
      return result;
    }
    
    void Skip(uint_t samples)
    {
      for (uint32_t skippedSamples = 0; skippedSamples < samples; )
      {
        const auto toSkip = std::min<uint32_t>(samples - skippedSamples, 1024);
        if (const auto res = ::usf_render(Emu.GetRaw(), nullptr, toSkip, nullptr))
        {
          throw MakeFormattedError(THIS_LINE, "USF: failed to skip: %1%", res);
        }
        skippedSamples += toSkip;
      }
    }
  private:
    void SetupSections(const std::list<Binary::Data::Ptr>& sections)
    {
      for (const auto& blob : sections)
      {
        if (-1 == ::usf_upload_section(Emu.GetRaw(), static_cast<const uint8_t*>(blob->Start()), blob->Size()))
        {
          throw MakeFormattedError(THIS_LINE, "USF: failed to upload_section");
        }
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
      if (const auto res = ::usf_render(Emu.GetRaw(), nullptr, 0, &freq))
      {
        throw MakeFormattedError(THIS_LINE, "USF: failed to detect frequency: %1%", res);
      }
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
      : Engine(data)
      , Iterator(Module::CreateStreamStateIterator(info))
      , State(Iterator->GetStateObserver())
      , Analyzer(CreateSoundAnalyzer())
      , SoundParams(Sound::RenderParameters::Create(params))
      , Target(Module::CreateFadingReceiver(std::move(params), std::move(info), State, std::move(target)))
      , Looped()
    {
      const auto frameDuration = SoundParams->FrameDuration();
      SamplesPerFrame = frameDuration.Get() * Engine.GetSoundFrequency() / frameDuration.PER_SECOND;
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

        auto data = Engine.Render(SamplesPerFrame);
        Analyzer->AddSoundData(data);
        Resampler->ApplyData(std::move(data));
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
      Engine.Reset();
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
        Looped = SoundParams->Looped();
        Resampler = Sound::CreateResampler(Engine.GetSoundFrequency(), SoundParams->SoundFreq(), Target);
      }
    }

    void SeekTune(uint_t frame)
    {
      uint_t current = State->Frame();
      if (frame < current)
      {
        Engine.Reset();
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        Engine.Skip(delta * SamplesPerFrame);
      }
    }
  private:
    USFEngine Engine;
    const StateIterator::Ptr Iterator;
    const Module::State::Ptr State;
    const SoundAnalyzer::Ptr Analyzer;
    uint_t SamplesPerFrame;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    const Sound::Receiver::Ptr Target;
    Sound::Receiver::Ptr Resampler;
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
      properties->SetValue(ATTR_PLATFORM, Platforms::NINTENDO_64);
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
    void AddSection(Binary::Data::Ptr data)
    {
      Require(!!data);
      Sections.emplace_back(std::move(data));
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
    std::list<Binary::Data::Ptr> Sections;
    XSF::MetaInformation::RWPtr Meta;
  };
  
  class Factory : public XSF::Factory
  {
  public:
    Holder::Ptr CreateSinglefileModule(const XSF::File& file, Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      Require(!file.PackedProgramSection);
      Require(!!file.ReservedSection);
      builder.AddSection(file.ReservedSection);
      if (file.Meta)
      {
        builder.AddMeta(*file.Meta);
      }
      return Holder::Create(builder.CaptureResult(), std::move(properties));
    }
    
    Holder::Ptr CreateMultifileModule(const XSF::File& file, const std::map<String, XSF::File>& additionalFiles, Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      MergeSections(file, additionalFiles, builder);
      MergeMeta(file, additionalFiles, builder);
      return Holder::Create(builder.CaptureResult(), std::move(properties));
    }
  private:
    /* https://bitbucket.org/zxtune/zxtune/wiki/USFFormat
    
    Loading a USF or USFlib/miniUSF

    1. initialize the ROM and save state to zero.
    2. if the USF contains a _lib tag (_libn not supported as of this version)
       recursively load the specified file starting from step 2
    3. load the ROM and save state, replacing any data with the same addresses that
       may have already been loaded

    By convention a file that includes a _lib tag is named with a .miniusf extension
    and a file that is included via a _lib tag is name with a .usflib extension.
    */
    static const uint_t MAX_LEVEL = 10;

    static void MergeSections(const XSF::File& data, const std::map<String, XSF::File>& additionalFiles, ModuleDataBuilder& dst, uint_t level = 1)
    {
      if (!data.Dependencies.empty() && level < MAX_LEVEL)
      {
        MergeSections(additionalFiles.at(data.Dependencies.front()), additionalFiles, dst, level + 1);
      }
      dst.AddSection(data.ReservedSection);
    }
    
    void MergeMeta(const XSF::File& data, const std::map<String, XSF::File>& additionalFiles, ModuleDataBuilder& dst, uint_t level = 1) const
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

#undef FILE_TAG
