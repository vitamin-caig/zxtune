/**
 *
 * @file
 *
 * @brief  USF chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "3rdparty/lazyusf2/usf/usf.h"

#include "module/players/platforms.h"
#include "module/players/properties_helper.h"
#include "module/players/streaming.h"
#include "module/players/xsf/all.h"
#include "module/players/xsf/xsf.h"

#include "debug/log.h"
#include "sound/resampler.h"

#include "contract.h"
#include "error_tools.h"
#include "make_ptr.h"

#include <list>

namespace Module::USF
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

    uint_t GetRefreshRate() const
    {
      return Meta->RefreshRate ? Meta->RefreshRate : 50;
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
    UsfHolder& operator=(const UsfHolder&) = delete;

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
    {
      SetupSections(data.Sections);
      if (data.Meta)
      {
        SetupEnvironment(*data.Meta);
      }
      DetectSoundFrequency();
      Dbg("Used sound frequency is {}", SoundFrequency);
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
      for (uint32_t doneSamples = 0; doneSamples < samples;)
      {
        const auto toRender = std::min<uint32_t>(samples - doneSamples, 1024);
        auto* const dst = safe_ptr_cast<short int*>(&result[doneSamples]);
        if (const auto* const res = ::usf_render(Emu.GetRaw(), dst, toRender, nullptr))
        {
          throw MakeFormattedError(THIS_LINE, "USF: failed to render: {}", res);
        }
        doneSamples += toRender;
      }
      return result;
    }

    void Skip(uint_t samples)
    {
      for (uint32_t skippedSamples = 0; skippedSamples < samples;)
      {
        const auto toSkip = std::min<uint32_t>(samples - skippedSamples, 1024);
        if (const auto* const res = ::usf_render(Emu.GetRaw(), nullptr, toSkip, nullptr))
        {
          throw MakeFormattedError(THIS_LINE, "USF: failed to skip: {}", res);
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
        if (tag.first == "_enablecompare"sv)
        {
          ::usf_set_compare(Emu.GetRaw(), true);
        }
        else if (tag.first == "_enablefifofull"sv)
        {
          ::usf_set_fifo_full(Emu.GetRaw(), true);
        }
      }
      ::usf_set_hle_audio(Emu.GetRaw(), true);
    }

    void DetectSoundFrequency()
    {
      int32_t freq = 0;
      if (const auto* const res = ::usf_render(Emu.GetRaw(), nullptr, 0, &freq))
      {
        throw MakeFormattedError(THIS_LINE, "USF: failed to detect frequency: {}", res);
      }
      Reset();
      SoundFrequency = freq;
    }

  private:
    UsfHolder Emu;
    uint_t SoundFrequency = 0;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(const ModuleData& data, uint_t samplerate)
      : Engine(data)
      , State(data.Meta->Duration)
      , Target(Sound::CreateResampler(Engine.GetSoundFrequency(), samplerate))
    {}

    Module::State GetState() const override
    {
      return State.Get();
    }

    Sound::Chunk Render() override
    {
      const auto avail = State.ConsumeUpTo(FRAME_DURATION);
      return Target->Apply(Engine.Render(GetSamples(avail)));
    }

    void Reset() override
    {
      State.Reset();
      Engine.Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < State.At())
      {
        Engine.Reset();
      }
      if (const auto toSkip = State.Seek(request))
      {
        Engine.Skip(GetSamples(toSkip));
      }
    }

  private:
    uint_t GetSamples(Time::Microseconds period) const
    {
      return period.Get() * Engine.GetSoundFrequency() / period.PER_SECOND;
    }

  private:
    USFEngine Engine;
    TimedState State;
    const Sound::Converter::Ptr Target;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModuleData::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {}

    Module::Information GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->Meta->Duration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(*Tune, samplerate);
    }

    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties)
    {
      PropertiesHelper props(*properties);
      if (tune->Meta)
      {
        tune->Meta->Dump(props);
      }
      props.SetPlatform(Platforms::NINTENDO_64);
      return MakePtr<Holder>(std::move(tune), std::move(properties));
    }

  private:
    const ModuleData::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  class ModuleDataBuilder : public XSF::MergeTarget
  {
  public:
    void AddProgramSection(Binary::Container::Ptr /*program*/) override
    {
      // the program section is not a part of the USF format
    }

    void AddReservedSection(Binary::Container::Ptr reserved) override
    {
      Require(!!reserved);
      Sections.emplace_back(std::move(reserved));
    }

    void AddMeta(const XSF::MetaInformation& meta) override
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
}  // namespace Module::USF

namespace Module::XSF
{
  Holder::Ptr CreateUSFModule(const File& file, const FilesMap& additionalFiles, Parameters::Container::Ptr properties)
  {
    USF::ModuleDataBuilder builder;
    MergeSectionsSingleLibrary(file, additionalFiles, builder);
    MergeMeta(file, additionalFiles, builder);
    return USF::Holder::Create(builder.CaptureResult(), std::move(properties));
  }
}  // namespace Module::XSF
