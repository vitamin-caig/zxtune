/**
 *
 * @file
 *
 * @brief  SPC support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/emulation/spc.h"
#include "module/players/duration.h"
#include "module/players/factory.h"
#include "module/players/platforms.h"
#include "module/players/properties_helper.h"
#include "module/players/properties_meta.h"
#include "module/players/streaming.h"

#include "binary/container_factories.h"
#include "binary/data.h"
#include "binary/view.h"
#include "core/plugin_attrs.h"
#include "debug/log.h"
#include "module/holder.h"
#include "module/information.h"
#include "module/renderer.h"
#include "parameters/container.h"
#include "sound/chunk.h"
#include "sound/receiver.h"
#include "sound/resampler.h"
#include "time/duration.h"
#include "time/instant.h"

#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"
#include "string_view.h"
#include "types.h"

#include "3rdparty/snesspc/snes_spc/SNES_SPC.h"
#include "3rdparty/snesspc/snes_spc/SPC_Filter.h"
#include "3rdparty/snesspc/snes_spc/blargg_common.h"

#include <exception>
#include <memory>
#include <utility>

namespace Binary
{
  class Container;
}  // namespace Binary

namespace Module::SPC
{
  const Debug::Stream Dbg("Core::SPCSupp");

  struct Model
  {
    using Ptr = std::shared_ptr<const Model>;

    const Binary::Data::Ptr Data;
    const Time::Milliseconds Duration;

    Model(Binary::View data, Time::Milliseconds duration)
      : Data(Binary::CreateContainer(data))
      , Duration(duration)
    {}
  };

  class SPC
  {
    static const uint_t SPC_DIVIDER = 1 << 12;
    static const uint_t C_7_FREQ = 2093;

  public:
    using Ptr = std::unique_ptr<SPC>;

    explicit SPC(Binary::View data)
      : Data(data)
    {
      CheckError(Spc.init());
      Reset();
    }

    void Reset()
    {
      Spc.reset();
      CheckError(Spc.load_spc(Data.Start(), Data.Size()));
      Spc.clear_echo();
      Spc.disable_surround(true);
      Filter.clear();
      Filter.set_gain(static_cast<int>(::SPC_Filter::gain_unit * 1.4));  // as in GME
    }

    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      Sound::Chunk result(samples);
      auto* const buffer = safe_ptr_cast< ::SNES_SPC::sample_t*>(result.data());
      const auto dataSize = static_cast<int>(samples * Sound::Sample::CHANNELS);
      CheckError(Spc.play(dataSize, buffer));
      Filter.run(buffer, dataSize);
      return result;
    }

    void Skip(uint_t samples)
    {
      CheckError(Spc.skip(static_cast<int>(samples * Sound::Sample::CHANNELS)));
    }

  private:
    inline static void CheckError(::blargg_err_t err)
    {
      Require(!err);  // TODO: detalize
    }

  private:
    const Binary::View Data;
    ::SNES_SPC Spc;
    ::SPC_Filter Filter;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  uint_t GetSamples(Time::Microseconds period)
  {
    return period.Get() * ::SNES_SPC::sample_rate / period.PER_SECOND;
  }

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr tune, Sound::Converter::Ptr target)
      : Tune(std::move(tune))
      , Engine(MakePtr<SPC>(*Tune->Data))
      , State(MakePtr<TimedState>(Tune->Duration))
      , Target(std::move(target))
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      const auto avail = State->ConsumeUpTo(FRAME_DURATION);
      return Target->Apply(Engine->Render(GetSamples(avail)));
    }

    void Reset() override
    {
      Engine->Reset();
      State->Reset();
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
    const Model::Ptr Tune;
    const SPC::Ptr Engine;
    const TimedState::Ptr State;
    const Sound::Converter::Ptr Target;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->Duration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(Tune, Sound::CreateResampler(::SNES_SPC::sample_rate, samplerate));
    }

  private:
    const Model::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  class DataBuilder : public Formats::Chiptune::SPC::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetRegisters(uint16_t /*pc*/, uint8_t /*a*/, uint8_t /*x*/, uint8_t /*y*/, uint8_t /*psw*/,
                      uint8_t /*sp*/) override
    {}

    void SetDumper(StringView dumper) override
    {
      Meta.SetComment(dumper);
    }

    void SetDumpDate(StringView date) override
    {
      Properties.SetDate(date);
    }

    void SetIntro(Time::Milliseconds duration) override
    {
      // Some tracks contain specified intro instead of loop
      if (!Loop)
      {
        Intro = duration;
        Properties.SetFadein(Intro);
      }
    }

    void SetLoop(Time::Milliseconds duration) override
    {
      Loop = duration;
    }

    void SetFade(Time::Milliseconds duration) override
    {
      if (duration)
      {
        Fade = duration;
        Properties.SetFadeout(duration);
      }
    }

    void SetRAM(Binary::View /*data*/) override {}

    void SetDSPRegisters(Binary::View /*data*/) override {}

    void SetExtraRAM(Binary::View /*data*/) override {}

    Time::Milliseconds GetDuration() const
    {
      return Intro + Loop + Fade;
    }

  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    Time::Milliseconds Intro;
    Time::Milliseconds Loop;
    Time::Milliseconds Fade;
  };

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::SPC::Parse(rawData, dataBuilder))
        {
          props.SetSource(*container);
          props.SetPlatform(Platforms::SUPER_NINTENDO_ENTERTAINMENT_SYSTEM);

          auto duration = dataBuilder.GetDuration();
          if (!duration.Get())
          {
            duration = GetDefaultDuration(params);
          }
          auto tune = MakePtr<Model>(*container, duration);

          return MakePtr<Holder>(std::move(tune), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create SPC: {}", e.what());
      }
      return {};
    }
  };
}  // namespace Module::SPC

namespace ZXTune
{
  void RegisterSPCSupport(PlayerPluginsRegistrator& registrator)
  {
    const auto ID = "SPC"_id;
    const uint_t CAPS = Capabilities::Module::Type::MEMORYDUMP | Capabilities::Module::Device::SPC700;

    auto decoder = Formats::Chiptune::CreateSPCDecoder();
    auto factory = MakePtr<Module::SPC::Factory>();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
