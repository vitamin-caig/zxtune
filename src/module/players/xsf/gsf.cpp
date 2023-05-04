/**
 *
 * @file
 *
 * @brief  GSF chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/xsf/gsf.h"
#include "module/players/xsf/gsf_rom.h"
#include "module/players/xsf/xsf.h"
#include "module/players/xsf/xsf_factory.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/compression/zlib_container.h>
#include <debug/log.h>
#include <module/attributes.h>
#include <module/players/platforms.h>
#include <module/players/streaming.h>
// 3rdparty includes
#include <3rdparty/mgba/defines.h>
#include <mgba-util/vfs.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/gba/core.h>
// std includes
#include <algorithm>

#undef min

namespace Module::GSF
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

    uint_t GetRefreshRate() const
    {
      return Meta->RefreshRate ? Meta->RefreshRate : 50;
    }
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
      if (count >= Tail.size())
      {
        SamplesToRender = count - Tail.size();
        Result.swap(Tail);
        Result.resize(count);
        this->postAudioBuffer = &OnRenderComplete;
      }
      else
      {
        SamplesToRender = 0;
        Result.resize(count);
        std::copy_n(Tail.begin(), count, Result.begin());
        std::copy(Tail.begin() + count, Tail.end(), Tail.begin());
        Tail.resize(Tail.size() - count);
      }
    }

    void SkipSamples(uint_t count)
    {
      Require(SamplesToRender == 0);
      Require(Result.empty());
      if (count >= Tail.size())
      {
        SamplesToSkip = count - Tail.size();
        this->postAudioBuffer = &OnSkipComplete;
      }
      else
      {
        SamplesToSkip = 0;
        std::copy(Tail.begin() + count, Tail.end(), Tail.begin());
        Tail.resize(Tail.size() - count);
      }
    }

    bool IsReady() const
    {
      return SamplesToRender == 0 && SamplesToSkip == 0;
    }

    Sound::Chunk CaptureResult()
    {
      return Sound::Chunk(std::move(Result));
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
      const auto done = std::min(blip_read_samples(left, dst, self->SamplesToRender, true),
                                 blip_read_samples(right, dst + 1, self->SamplesToRender, true));
      self->SamplesToRender -= done;
      if (!self->SamplesToRender)
      {
        self->KeepFrameTail(left, right);
      }
    }

    static void OnSkipComplete(struct mAVStream* in, blip_t* left, blip_t* right)
    {
      AVStream* const self = safe_ptr_cast<AVStream*>(in);
      int16_t dummy[AUDIO_BUFFER_SIZE];
      const auto toSkip = std::min<uint_t>(self->SamplesToSkip, AUDIO_BUFFER_SIZE);
      const auto done =
          std::min(blip_read_samples(left, dummy, toSkip, false), blip_read_samples(right, dummy, toSkip, false));
      self->SamplesToSkip -= done;
      if (!self->SamplesToSkip)
      {
        self->KeepFrameTail(left, right);
      }
    }

    void KeepFrameTail(blip_t* left, blip_t* right)
    {
      if (const auto avail = std::min(blip_samples_avail(left), blip_samples_avail(right)))
      {
        Tail.resize(avail);
        auto* dst = safe_ptr_cast<int16_t*>(Tail.data());
        blip_read_samples(left, dst, avail, true);
        blip_read_samples(right, dst + 1, avail, true);
      }
    }

  private:
    Sound::Chunk Result;
    uint_t SamplesToRender = 0;
    uint_t SamplesToSkip = 0;
    Sound::Chunk Tail;
  };

  class GbaCore
  {
  public:
    explicit GbaCore(const GbaRom& rom)
      : Core(GBACoreCreate())
    {
      Core->init(Core);
      mCoreInitConfig(Core, NULL);
      // core owns rom file memory, so copy it
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
    {}

    void SetFrequency(uint_t freq)
    {
      Core.InitSound(freq, Stream);
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

  const auto FRAME_DURATION = Time::Milliseconds(100);

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(const ModuleData& data, uint_t samplerate)
      : Engine(MakePtr<GbaEngine>(data))
      , State(MakePtr<TimedState>(data.Meta->Duration))
      , SoundFrequency(samplerate)
    {
      Engine->SetFrequency(samplerate);
    }

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
      return period.Get() * SoundFrequency / period.PER_SECOND;
    }

  private:
    const GbaEngine::Ptr Engine;
    const TimedState::Ptr State;
    uint_t SoundFrequency = 0;
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
      return MakePtr<Renderer>(*Tune, samplerate);
    }

    static Ptr Create(ModuleData::Ptr tune, Parameters::Container::Ptr properties)
    {
      if (tune->Meta)
      {
        tune->Meta->Dump(*properties);
      }
      properties->SetValue(ATTR_PLATFORM, Platforms::GAME_BOY_ADVANCE.to_string());
      return MakePtr<Holder>(std::move(tune), std::move(properties));
    }

  private:
    const ModuleData::Ptr Tune;
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
      // don't know anything about reserved section state
      return Holder::Create(builder.CaptureResult(), std::move(properties));
    }

    Holder::Ptr CreateMultifileModule(const XSF::File& file, const XSF::FilesMap& additionalFiles,
                                      Parameters::Container::Ptr properties) const override
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

    static void MergeRom(const XSF::File& data, const XSF::FilesMap& additionalFiles, ModuleDataBuilder& dst,
                         uint_t level = 1)
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
}  // namespace Module::GSF
