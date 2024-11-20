/**
 *
 * @file
 *
 * @brief  SSF/DSF chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/xsf/sdsf.h"

#include "module/players/platforms.h"
#include "module/players/streaming.h"
#include "module/players/xsf/xsf_file.h"
#include "module/players/xsf/xsf_metainformation.h"

#include "binary/compression/zlib_container.h"
#include "binary/container.h"
#include "debug/log.h"
#include "module/attributes.h"
#include "module/holder.h"
#include "module/information.h"
#include "module/renderer.h"
#include "parameters/container.h"
#include "sound/chunk.h"
#include "sound/receiver.h"
#include "sound/resampler.h"
#include "strings/array.h"
#include "time/duration.h"
#include "time/instant.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"
#include "string_view.h"

#include "3rdparty/ht/Core/emuconfig.h"
#include "3rdparty/ht/Core/sega.h"

#include <algorithm>
#include <list>
#include <memory>
#include <utility>

namespace Module::SDSF
{
  const Debug::Stream Dbg("Module::SDSF");

  struct ModuleData
  {
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;

    ModuleData() = default;
    ModuleData(const ModuleData&) = delete;

    uint_t Version = 0;
    std::list<Binary::Data::Ptr> Sections;
    XSF::MetaInformation::Ptr Meta;

    uint_t GetRefreshRate() const
    {
      if (Meta && Meta->RefreshRate)
      {
        return Meta->RefreshRate;
      }
      else
      {
        return 60;  // NTSC by default
      }
    }
  };

  class HTLibrary
  {
  private:
    HTLibrary()
    {
      Require(0 == ::sega_init());
    }

  public:
    enum class Version
    {
      Saturn = 1,
      Dreamcast = 2,
    };

    static std::unique_ptr<uint8_t[]> CreateSega(Version version)
    {
      std::unique_ptr<uint8_t[]> res(new uint8_t[::sega_get_state_size(static_cast<uint8>(version))]);
      ::sega_clear_state(res.get(), static_cast<uint8>(version));
      return res;
    }

    static uint32_t GetMemoryEnd(Version vers)
    {
      if (vers == Version::Saturn)
      {
        return 0x80000;
      }
      else
      {
        return 0x200000;
      }
    }

    static const HTLibrary& Instance()
    {
      static const HTLibrary instance;
      return instance;
    }
  };

  class SegaEngine
  {
  public:
    enum
    {
      SAMPLERATE = 44100
    };

    void Initialize(const ModuleData& data)
    {
      Vers = static_cast<HTLibrary::Version>(data.Version - 0x10);
      Emu = HTLibrary::Instance().CreateSega(Vers);

      const bool dry = true;
      const bool dsp = true;
      ::sega_enable_dry(Emu.get(), dry || !dsp);
      ::sega_enable_dsp(Emu.get(), dsp);

      SetupSections(data.Sections);
    }

    Sound::Chunk Render(uint_t samples)
    {
      Sound::Chunk result(samples);
      for (uint32_t doneSamples = 0; doneSamples < samples;)
      {
        uint32_t toRender = samples - doneSamples;
        const auto res =
            ::sega_execute(Emu.get(), 0x7fffffff, safe_ptr_cast<short int*>(&result[doneSamples]), &toRender);
        Require(res >= 0);
        Require(toRender != 0);
        doneSamples += toRender;
      }
      return result;
    }

    void Skip(uint_t samples)
    {
      for (uint32_t skippedSamples = 0; skippedSamples < samples;)
      {
        uint32_t toSkip = samples - skippedSamples;
        const auto res = ::sega_execute(Emu.get(), 0x7fffffff, nullptr, &toSkip);
        Require(res >= 0);
        Require(toSkip != 0);
        skippedSamples += toSkip;
      }
    }

  private:
    void SetupSections(const std::list<Binary::Data::Ptr>& sections)
    {
      for (const auto& packed : sections)
      {
        const auto unpackedSection = Binary::Compression::Zlib::Decompress(*packed);
        const auto rawSize = unpackedSection->Size();
        Require(rawSize > sizeof(le_uint32_t));
        auto* const rawStart = static_cast<le_uint32_t*>(const_cast<void*>(unpackedSection->Start()));
        const auto toCopy = FixupSection(rawStart, rawSize);
        // TODO: make input const
        Dbg("Section {} -> {}  @ 0x{:08x}", packed->Size(), toCopy, *rawStart);
        Require(0 == ::sega_upload_program(Emu.get(), rawStart, toCopy));
      }
    }

    std::size_t FixupSection(le_uint32_t* data, std::size_t size) const
    {
      const uint32_t start = *data & 0x7fffff;
      *data = start;
      const uint32_t end = start + (size - sizeof(start));
      const uint32_t realEnd = std::min(end, HTLibrary::GetMemoryEnd(Vers));
      return sizeof(start) + (realEnd - start);
    }

  private:
    HTLibrary::Version Vers;
    std::unique_ptr<uint8_t[]> Emu;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  uint_t GetSamples(Time::Microseconds period)
  {
    return period.Get() * SegaEngine::SAMPLERATE / period.PER_SECOND;
  }

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr data, Sound::Converter::Ptr target)
      : Data(std::move(data))
      , State(MakePtr<TimedState>(Data->Meta->Duration))
      , Target(std::move(target))
    {
      Engine.Initialize(*Data);
    }

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      const auto avail = State->ConsumeUpTo(FRAME_DURATION);
      return Target->Apply(Engine.Render(GetSamples(avail)));
    }

    void Reset() override
    {
      State->Reset();
      Engine.Initialize(*Data);
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < State->At())
      {
        Engine.Initialize(*Data);
      }
      if (const auto toSkip = State->Seek(request))
      {
        Engine.Skip(GetSamples(toSkip));
      }
    }

  private:
    const ModuleData::Ptr Data;
    const TimedState::Ptr State;
    SegaEngine Engine;
    const Sound::Converter::Ptr Target;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModuleData::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->Meta->Duration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(Tune, Sound::CreateResampler(SegaEngine::SAMPLERATE, samplerate));
    }

    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties)
    {
      if (tune->Meta)
      {
        tune->Meta->Dump(*properties);
      }
      properties->SetValue(ATTR_PLATFORM, tune->Version == 0x11 ? Platforms::SEGA_SATURN : Platforms::DREAMCAST);
      return MakePtr<Holder>(std::move(tune), std::move(properties));
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

    ModuleData::Ptr CaptureResult(uint_t version)
    {
      auto res = MakeRWPtr<ModuleData>();
      res->Version = version;
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
      Require(!!file.PackedProgramSection);
      Require(!file.ReservedSection);
      builder.AddSection(file.PackedProgramSection);
      if (file.Meta)
      {
        builder.AddMeta(*file.Meta);
      }
      return Holder::Create(builder.CaptureResult(file.Version), std::move(properties));
    }

    Holder::Ptr CreateMultifileModule(const XSF::File& file, const XSF::FilesMap& additionalFiles,
                                      Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      MergeSections(file, additionalFiles, builder);
      MergeMeta(file, additionalFiles, builder);
      return Holder::Create(builder.CaptureResult(file.Version), std::move(properties));
    }

  private:
    /* https://bitbucket.org/zxtune/zxtune/wiki/MiniPSF

    The proper way to load a minipsf is as follows:
    - Load the executable data from the minipsf - this becomes the current executable.
    - Check for the presence of a "_lib" tag. If present:
      - RECURSIVELY load the executable data from the given library file. (Make sure to limit recursion to avoid
    crashing - I usually limit it to 10 levels)
      - Make the _lib executable the current one.
      - If applicable, we will use the initial program counter/stack pointer from the _lib executable.
      - Superimpose the originally loaded minipsf executable on top of the current executable. If applicable, use the
    start address and size to determine where to .
    - Check for the presence of "_libN" tags for N=2 and up (use "_lib%d")
      - RECURSIVELY load and superimpose all these EXEs on top of the current EXE. Do not modify the current program
    counter or stack pointer.
      - Start at N=2. Stop at the first tag name that doesn't exist.
    - (done)
    */
    static const uint_t MAX_LEVEL = 10;

    static void MergeSections(const XSF::File& data, const XSF::FilesMap& additionalFiles, ModuleDataBuilder& dst,
                              uint_t level = 1)
    {
      auto it = data.Dependencies.begin();
      const auto lim = data.Dependencies.end();
      if (it != lim && level < MAX_LEVEL)
      {
        MergeSections(additionalFiles.at(*it), additionalFiles, dst, level + 1);
      }
      dst.AddSection(data.PackedProgramSection);
      if (it != lim && level < MAX_LEVEL)
      {
        for (++it; it != lim; ++it)
        {
          MergeSections(additionalFiles.at(*it), additionalFiles, dst, level + 1);
        }
      }
    }

    static void MergeMeta(const XSF::File& data, const XSF::FilesMap& additionalFiles, ModuleDataBuilder& dst,
                          uint_t level = 1)
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

  XSF::Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::SDSF
