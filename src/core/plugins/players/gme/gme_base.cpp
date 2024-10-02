/**
 *
 * @file
 *
 * @brief  Game Music Emu-based formats support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/gme/kss_supp.h"
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/players/plugin.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <binary/compression/zlib_stream.h>
#include <binary/format_factories.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/multitrack/decoders.h>
#include <math/numeric.h>
#include <module/attributes.h>
#include <module/players/duration.h>
#include <module/players/platforms.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>
#include <strings/optimize.h>
// std includes
#include <map>
// 3rdparty includes
#include <3rdparty/gme/gme/Gbs_Emu.h>
#include <3rdparty/gme/gme/Gym_Emu.h>
#include <3rdparty/gme/gme/Hes_Emu.h>
#include <3rdparty/gme/gme/Kss_Emu.h>
#include <3rdparty/gme/gme/Nsf_Emu.h>
#include <3rdparty/gme/gme/Nsfe_Emu.h>
#include <3rdparty/gme/gme/Sap_Emu.h>
#include <3rdparty/gme/gme/Vgm_Emu.h>

namespace Module::GME
{
  const Debug::Stream Dbg("Core::GMESupp");

  using EmuPtr = std::unique_ptr< ::Music_Emu>;

  using EmuCreator = EmuPtr (*)();

  template<class EmuType>
  EmuPtr Create()
  {
    return EmuPtr(new EmuType());
  }

  inline void CheckError(::blargg_err_t err)
  {
    if (err)
    {
      throw std::runtime_error(err);
    }
  }

  using TimeBase = Time::Millisecond;

  struct GMETune
  {
    using Ptr = std::shared_ptr<GMETune>;

    GMETune(EmuCreator create, Binary::Data::Ptr data, uint_t track)
      : CreateEmu(create)
      , Data(std::move(data))
      , Track(track)
    {}

    const EmuCreator CreateEmu;
    const Binary::Data::Ptr Data;
    const uint_t Track;
    Time::Milliseconds Duration;

    ::track_info_t GetInfo() const
    {
      const uint_t FAKE_SOUND_FREQUENCY = 30000;
      const auto emu = CreateEmu();
      CheckError(emu->set_sample_rate(FAKE_SOUND_FREQUENCY));
      CheckError(emu->load_mem(Data->Start(), Data->Size()));
      ::track_info_t info;
      CheckError(emu->track_info(&info, Track));
      return info;
    }

    void SetDuration(const ::track_info_t& info, const Parameters::Accessor& params)
    {
      if (info.length > 0)
      {
        Duration = Time::Duration<TimeBase>(info.length);
      }
      else if (info.loop_length > 0)
      {
        Duration = Time::Duration<TimeBase>(info.intro_length + info.loop_length);
      }
      else
      {
        Duration = GetDefaultDuration(params);
      }
    }
  };

  class GME
  {
  public:
    GME(const GMETune& tune, uint_t samplerate)
      : Emu(tune.CreateEmu())
      , SoundFreq(samplerate)
      , Track(tune.Track)
    {
      CheckError(Emu->set_sample_rate(samplerate));
      CheckError(Emu->load_mem(tune.Data->Start(), tune.Data->Size()));
      Reset();
    }

    void Reset()
    {
      CheckError(Emu->start_track(Track));
    }

    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      Sound::Chunk result(samples);
      auto* const buffer = safe_ptr_cast< ::Music_Emu::sample_t*>(result.data());
      CheckError(Emu->play(static_cast<int>(samples * Sound::Sample::CHANNELS), buffer));
      return result;
    }

    void Skip(uint_t samples)
    {
      CheckError(Emu->skip(samples));
    }

    uint_t GetSoundFreq() const
    {
      return SoundFreq;
    }

  private:
    const EmuPtr Emu;
    const uint_t SoundFreq;
    const uint_t Track;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(GMETune::Ptr tune, uint_t samplerate)
      : Tune(std::move(tune))
      , State(MakePtr<TimedState>(Tune->Duration))
      , Engine(*Tune, samplerate)
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      const auto avail = State->ConsumeUpTo(FRAME_DURATION);
      return Engine.Render(GetSamples(avail));
    }

    void Reset() override
    {
      try
      {
        State->Reset();
        Engine.Reset();
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      try
      {
        SeekTune(request);
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

  private:
    uint_t GetSamples(Time::Microseconds period) const
    {
      return period.Get() * Engine.GetSoundFreq() / period.PER_SECOND;
    }

    void SeekTune(Time::AtMillisecond request)
    {
      if (request < State->At())
      {
        Engine.Reset();
      }
      if (const auto toSkip = State->Seek(request))
      {
        Engine.Skip(GetSamples(toSkip));
      }
    }

  private:
    const GMETune::Ptr Tune;
    const TimedState::Ptr State;
    GME Engine;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(GMETune::Ptr tune, Parameters::Accessor::Ptr props)
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
      try
      {
        return MakePtr<Renderer>(Tune, samplerate);
      }
      catch (const std::exception& e)
      {
        throw Error(THIS_LINE, e.what());
      }
    }

  private:
    const GMETune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };

  // TODO: rework, extract GYM parsing code to Formats library
  Binary::Data::Ptr DefaultDataCreator(Binary::View data)
  {
    return Binary::CreateContainer(data);
  }

  using PlatformDetector = StringView (*)(Binary::View);

  struct PluginDescription
  {
    const ZXTune::PluginId Id;
    const uint_t ChiptuneCaps;
    const EmuCreator CreateEmu;
    const decltype(&DefaultDataCreator) CreateData;
    const PlatformDetector DetectPlatform;
  };

  namespace GYM
  {
    Binary::Data::Ptr CreateData(Binary::View data)
    {
      Binary::DataInputStream input(data);
      Binary::DataBuilder output(data.Size());
      const std::size_t packedSizeOffset = 424;
      output.Add(input.ReadData(packedSizeOffset));
      if (const auto packedSize = input.Read<le_uint32_t>())
      {
        output.Add<le_uint32_t>(0);
        Binary::Compression::Zlib::Decompress(input, output);
      }
      else
      {
        output.Add<le_uint32_t>(0);
        output.Add(input.ReadRestData());
      }
      return output.CaptureResult();
    }
  }  // namespace GYM

  void GetProperties(const ::track_info_t& info, PropertiesHelper& props)
  {
    const auto system = Strings::OptimizeAscii(info.system);
    const auto song = Strings::OptimizeAscii(info.song);
    const auto game = Strings::OptimizeAscii(info.game);
    const auto author = Strings::OptimizeAscii(info.author);
    const auto comment = Strings::OptimizeAscii(info.comment);
    const auto copyright = Strings::OptimizeAscii(info.copyright);
    const auto dumper = Strings::OptimizeAscii(info.dumper);

    props.SetComputer(system);
    props.SetTitle(game);
    props.SetTitle(song);
    props.SetProgram(game);
    props.SetAuthor(dumper);
    props.SetAuthor(author);
    props.SetComment(copyright);
    props.SetComment(comment);
  }

  class MultitrackFactory : public Module::MultitrackFactory
  {
  public:
    explicit MultitrackFactory(PluginDescription desc)
      : Desc(std::move(desc))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params,
                                     const Formats::Multitrack::Container& container,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        auto data = Desc.CreateData(container);
        props.SetPlatform(Desc.DetectPlatform(*data));
        auto tune = MakePtr<GMETune>(Desc.CreateEmu, std::move(data), container.StartTrackIndex());

        const auto info = tune->GetInfo();
        GetProperties(info, props);
        tune->SetDuration(info, params);

        return MakePtr<Holder>(std::move(tune), std::move(properties));
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create {}: {}", Desc.Id, e.what());
      }
      return {};
    }

  private:
    const PluginDescription Desc;
  };

  class SingletrackFactory : public Module::ExternalParsingFactory
  {
  public:
    explicit SingletrackFactory(PluginDescription desc)
      : Desc(std::move(desc))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Formats::Chiptune::Container& container,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        auto data = Desc.CreateData(container);
        props.SetPlatform(Desc.DetectPlatform(*data));
        auto tune = MakePtr<GMETune>(Desc.CreateEmu, std::move(data), 0);
        const auto info = tune->GetInfo();
        GetProperties(info, props);
        tune->SetDuration(info, params);

        return MakePtr<Holder>(std::move(tune), std::move(properties));
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create {}: {}", Desc.Id, e.what());
      }
      return {};
    }

  private:
    const PluginDescription Desc;
  };

  struct MultitrackPluginDescription
  {
    using MultitrackDecoderCreator = Formats::Multitrack::Decoder::Ptr (*)();

    PluginDescription Desc;
    const MultitrackDecoderCreator CreateMultitrackDecoder;
  };

  using ZXTune::operator""_id;

  // clang-format off
  const MultitrackPluginDescription MULTITRACK_PLUGINS[] =
  {
    //nsf
    {
      {
        "NSF"_id,
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::RP2A0X,
        &Create< ::Nsf_Emu>,
        &DefaultDataCreator,
        [](Binary::View) -> StringView {return Platforms::NINTENDO_ENTERTAINMENT_SYSTEM;}
      },
      &Formats::Multitrack::CreateNSFDecoder,
    },
    //nsfe
    {
      {
        "NSFE"_id,
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::RP2A0X,
        &Create< ::Nsfe_Emu>,
        &DefaultDataCreator,
        [](Binary::View) -> StringView {return Platforms::NINTENDO_ENTERTAINMENT_SYSTEM;}
      },
      &Formats::Multitrack::CreateNSFEDecoder,
    },
    //gbs
    {
      {
        "GBS"_id,
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::LR35902,
        &Create< ::Gbs_Emu>,
        &DefaultDataCreator,
        [](Binary::View) -> StringView {return Platforms::GAME_BOY;}
      },
      &Formats::Multitrack::CreateGBSDecoder,
    },
    //kssx
    {
      {
        "KSSX"_id,
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::MULTI,
        &Create< ::Kss_Emu>,
        &DefaultDataCreator,
        &KSS::DetectPlatform
      },
      &Formats::Multitrack::CreateKSSXDecoder,
    },
    //hes
    {
      {
        "HES"_id,
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::HUC6270,
        &Create< ::Hes_Emu>,
        &DefaultDataCreator,
        [](Binary::View) -> StringView {return Platforms::PC_ENGINE;}
      },
      &Formats::Multitrack::CreateHESDecoder,
    },
  };
  // clang-format on

  struct SingletrackPluginDescription
  {
    using ChiptuneDecoderCreator = Formats::Chiptune::Decoder::Ptr (*)();

    PluginDescription Desc;
    const ChiptuneDecoderCreator CreateChiptuneDecoder;
  };

  // clang-format off
  const SingletrackPluginDescription SINGLETRACK_PLUGINS[] =
  {
    //gym
    {
      {
        "GYM"_id,
        ZXTune::Capabilities::Module::Type::STREAM | ZXTune::Capabilities::Module::Device::MULTI,
        &Create< ::Gym_Emu>,
        &GYM::CreateData,
        [](Binary::View) -> StringView {return Platforms::SEGA_GENESIS;}
      },
      &Formats::Chiptune::CreateGYMDecoder
    },
    //kss
    {
      {
        "KSS"_id,
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::MULTI,
        &Create< ::Kss_Emu>,
        &DefaultDataCreator,
        &KSS::DetectPlatform
      },
      &Formats::Chiptune::CreateKSSDecoder
    }
  };
  // clang-format on
}  // namespace Module::GME

namespace ZXTune
{
  void RegisterMultitrackGMEPlugins(PlayerPluginsRegistrator& players, ArchivePluginsRegistrator& archives)
  {
    for (const auto& desc : Module::GME::MULTITRACK_PLUGINS)
    {
      auto decoder = desc.CreateMultitrackDecoder();
      auto factory = MakePtr<Module::GME::MultitrackFactory>(desc.Desc);
      {
        auto plugin = CreatePlayerPlugin(desc.Desc.Id, desc.Desc.ChiptuneCaps, decoder, factory);
        players.RegisterPlugin(std::move(plugin));
      }
      {
        auto plugin = CreateArchivePlugin(desc.Desc.Id, std::move(decoder), std::move(factory));
        archives.RegisterPlugin(std::move(plugin));
      }
    }
  }

  void RegisterSingletrackGMEPlugins(PlayerPluginsRegistrator& registrator)
  {
    for (const auto& desc : Module::GME::SINGLETRACK_PLUGINS)
    {
      auto factory = MakePtr<Module::GME::SingletrackFactory>(desc.Desc);
      auto decoder = desc.CreateChiptuneDecoder();
      auto plugin = CreatePlayerPlugin(desc.Desc.Id, desc.Desc.ChiptuneCaps, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }

  void RegisterGMEPlugins(PlayerPluginsRegistrator& playerRegistrator, ArchivePluginsRegistrator& archiveRegistrator)
  {
    RegisterMultitrackGMEPlugins(playerRegistrator, archiveRegistrator);
    RegisterSingletrackGMEPlugins(playerRegistrator);
  }
}  // namespace ZXTune
