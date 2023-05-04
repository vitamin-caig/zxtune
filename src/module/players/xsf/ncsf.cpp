/**
 *
 * @file
 *
 * @brief  NCSF chiptune factory implementation
 *
 * @author liushuyu011@gmail.com
 *
 **/

// local includes
#include "module/players/xsf/ncsf.h"
#include "module/players/xsf/memory_region.h"
#include "module/players/xsf/xsf.h"
#include "module/players/xsf/xsf_factory.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/compression/zlib_container.h>
#include <debug/log.h>
#include <formats/chiptune/emulation/nitrocomposersoundformat.h>
#include <math/bitops.h>
#include <module/attributes.h>
#include <module/players/platforms.h>
#include <module/players/streaming.h>
#include <sound/resampler.h>
// std includes
#include <list>
// 3rdparty includes
#include <3rdparty/sseqplayer/Player.h>
#include <3rdparty/sseqplayer/SDAT.h>

namespace Module::NCSF
{
  const Debug::Stream Dbg("Module::NCSF");

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

  class NCSFEngine
  {
  public:
    using Ptr = std::shared_ptr<NCSFEngine>;

    NCSFEngine(const ModuleData& data, uint32_t sampleRate)
      : SoundFrequency(sampleRate)
    {
      if (!data.PackedProgramSections.empty())
      {
        SetupRom(data.PackedProgramSections);
      }
      if (!data.ReservedSections.empty())
      {
        SetupState(data.ReservedSections);
      }

      PseudoFile file;
      file.data = &Rom.Data;
      SDat.reset(new SDAT(file, SSeq));
      NCSFPlayer.sampleRate = SoundFrequency;
      NCSFPlayer.interpolation = INTERPOLATION_SINC;
      NCSFPlayer.Setup(SDat->sseq.get());
      NCSFPlayer.Timer();  // start the emulation timer
    }

    void Reset()
    {
      // we are not killing the sound since we are doing a quick reset
      NCSFPlayer.Stop(false);
      NCSFPlayer.Setup(SDat->sseq.get());
      NCSFPlayer.Timer();
    }

    ~NCSFEngine()
    {
      NCSFPlayer.Stop(true);  // stop the emulation and shut off the sound output
                              // (true = kill sound)
    }

    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Invalid sound channels count");
      static_assert(Sound::Sample::MID == 0, "Invalid sound sample type");
      static_assert(Sound::Sample::MAX == 32767 && Sound::Sample::MIN == -32768, "Invalid sound sample type");

      Sound::Chunk res(samples);
      SampleBuffer.resize(samples * 2 * sizeof(int16_t), 0);
      NCSFPlayer.GenerateSamples(SampleBuffer, 0, samples);
      memmove(res.data(), SampleBuffer.data(), samples * 2 * sizeof(int16_t));
      return res;
    }

    uint32_t GetSampleFrequency()
    {
      return SoundFrequency;
    }

    void Skip(uint_t samples)
    {
      // turn off interpolation during the fast-forward to speed up the emulation
      NCSFPlayer.interpolation = INTERPOLATION_NONE;
      SampleBuffer.resize(samples * 2 * sizeof(int16_t), 0);
      NCSFPlayer.GenerateSamples(SampleBuffer, 0, samples);
      NCSFPlayer.interpolation = INTERPOLATION_SINC;
    }

  private:
    void SetupRom(const std::list<Binary::Container::Ptr>& blocks)
    {
      ChunkBuilder builder;
      for (const auto& block : blocks)
      {
        const auto unpacked = Binary::Compression::Zlib::Decompress(*block);
        Formats::Chiptune::NitroComposerSoundFormat::ParseRom(*unpacked, builder);
      }
      // possibly, emulation writes to ROM are, so copy it
      Rom = builder.CaptureResult();
      // required power of 2 size
      const auto alignedRomSize = uint32_t(1) << Math::Log2(Rom.Data.size());
      Rom.Data.resize(alignedRomSize);
    }

    void SetupState(const std::list<Binary::Container::Ptr>& blocks)
    {
      ChunkBuilder builder;
      for (const auto& block : blocks)
      {
        SSeq = Formats::Chiptune::NitroComposerSoundFormat::ParseState(*block);
      }
    }

  private:
    class ChunkBuilder : public Formats::Chiptune::NitroComposerSoundFormat::Builder
    {
    public:
      void SetChunk(uint32_t offset, Binary::View content) override
      {
        Result.Update(offset, content);
      }

      MemoryRegion CaptureResult()
      {
        return std::move(Result);
      }

    private:
      MemoryRegion Result;
    };

  private:
    const uint32_t SoundFrequency;
    ::Player NCSFPlayer;
    std::unique_ptr<SDAT> SDat;
    std::vector<uint8_t> SampleBuffer;
    uint32_t SSeq = 0;
    MemoryRegion Rom;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModuleData::Ptr data, uint_t samplerate)
      : Data(std::move(data))
      , State(MakePtr<TimedState>(Data->Meta->Duration))
      , Engine(MakePtr<NCSFEngine>(*Data, samplerate))
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      const auto avail = State->Consume(FRAME_DURATION);
      return Engine->Render(GetSamples(avail));
    }

    void Reset() override
    {
      State->Reset();
      Engine->Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < State->At())
      {
        Engine->Reset();
      }
      if (const auto toSkip = State->Seek(request))
      {
        Engine->Skip(GetSamples(toSkip));
      }
    }

  private:
    uint_t GetSamples(Time::Microseconds period) const
    {
      return period.Get() * Engine->GetSampleFrequency() / period.PER_SECOND;
    }

  private:
    const ModuleData::Ptr Data;
    const TimedState::Ptr State;
    NCSFEngine::Ptr Engine;
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
      return MakePtr<Renderer>(Tune, samplerate);
    }

    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties)
    {
      if (tune->Meta)
      {
        tune->Meta->Dump(*properties);
      }
      properties->SetValue(ATTR_PLATFORM, Platforms::NINTENDO_DS.to_string());
      return MakePtr<Holder>(std::move(tune), std::move(properties));
    }

  private:
    const ModuleData::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  class ModuleDataBuilder
  {
  public:
    ModuleDataBuilder()
      : Result(MakeRWPtr<ModuleData>())
    {}

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

  class Factory : public XSF::Factory
  {
  public:
    Holder::Ptr CreateSinglefileModule(const XSF::File& file, Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      if (file.PackedProgramSection)
      {
        builder.AddProgramSection(file.PackedProgramSection);
      }
      if (file.ReservedSection)
      {
        builder.AddReservedSection(file.ReservedSection);
      }
      if (file.Meta)
      {
        builder.AddMeta(*file.Meta);
      }
      return Holder::Create(builder.CaptureResult(), std::move(properties));
    }

    Holder::Ptr CreateMultifileModule(const XSF::File& file, const XSF::FilesMap& additionalFiles,
                                      Parameters::Container::Ptr properties) const override
    {
      ModuleDataBuilder builder;
      MergeSections(file, additionalFiles, builder);
      MergeMeta(file, additionalFiles, builder);
      return Holder::Create(builder.CaptureResult(), std::move(properties));
    }

  private:
    static const uint_t MAX_LEVEL = 10;

    static void MergeSections(const XSF::File& data, const XSF::FilesMap& additionalFiles, ModuleDataBuilder& dst,
                              uint_t level = 1)
    {
      if (!data.Dependencies.empty() && level < MAX_LEVEL)
      {
        MergeSections(additionalFiles.at(data.Dependencies.front()), additionalFiles, dst, level + 1);
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

  Module::Factory::Ptr CreateFactory()
  {
    return XSF::CreateFactory(MakePtr<Factory>());
  }
}  // namespace Module::NCSF
