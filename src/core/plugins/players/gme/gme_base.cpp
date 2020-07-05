/**
* 
* @file
*
* @brief  Game Music Emu-based formats support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/gme/kss_supp.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
#include <binary/compression/zlib_stream.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/multitrack/decoders.h>
#include <formats/chiptune/multitrack/decoders.h>
#include <formats/chiptune/multitrack/multitrack.h>
#include <math/numeric.h>
#include <module/attributes.h>
#include <module/players/analyzer.h>
#include <module/players/duration.h>
#include <module/players/properties_helper.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
#include <strings/optimize.h>
//std includes
#include <map>
//boost includes
#include <boost/algorithm/string/predicate.hpp>
//3rdparty
#include <3rdparty/gme/gme/Gbs_Emu.h>
#include <3rdparty/gme/gme/Hes_Emu.h>
#include <3rdparty/gme/gme/Nsf_Emu.h>
#include <3rdparty/gme/gme/Nsfe_Emu.h>
#include <3rdparty/gme/gme/Sap_Emu.h>
#include <3rdparty/gme/gme/Vgm_Emu.h>
#include <3rdparty/gme/gme/Gym_Emu.h>
#include <3rdparty/gme/gme/Kss_Emu.h>
//text includes
#include <module/text/platforms.h>

#define FILE_TAG 513E65A8

namespace Module
{
namespace GME
{
  const Debug::Stream Dbg("Core::GMESupp");

  typedef std::shared_ptr< ::Music_Emu> EmuPtr;

  typedef EmuPtr (*EmuCreator)();
  
  template<class EmuType>
  EmuPtr Create()
  {
    return EmuPtr(new EmuType());
  }

  inline static void CheckError(::blargg_err_t err)
  {
    if (err)
    {
      throw std::runtime_error(err);
    }
  }

  struct GMETune
  {
    using Ptr = std::shared_ptr<GMETune>;

    GMETune(EmuCreator create, Dump data, uint_t track)
      : CreateEmu(create)
      , Data(std::move(data))
      , Track(track)
    {
    }

    EmuCreator CreateEmu;
    Dump Data;
    uint_t Track;

    ::track_info_t GetInfo() const
    {
      const uint_t FAKE_SOUND_FREQUENCY = 30000;
      const auto emu = CreateEmu();
      CheckError(emu->set_sample_rate(FAKE_SOUND_FREQUENCY));
      CheckError(emu->load_mem(Data.data(), Data.size()));
      ::track_info_t info;
      CheckError(emu->track_info(&info, Track));
      return info;
    }
  };
  
  class GME
  {
  public:
    explicit GME(GMETune::Ptr tune)
      : Tune(std::move(tune))
      , SoundFreq(0)
    {
    }
    
    void Reset()
    {
      CheckError(Emu->start_track(Tune->Track));
    }
    
    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      Sound::Chunk result(samples);
      ::Music_Emu::sample_t* const buffer = safe_ptr_cast< ::Music_Emu::sample_t*>(result.data());
      CheckError(Emu->play(samples * Sound::Sample::CHANNELS, buffer));
      return result;
    }
    
    void Skip(uint_t samples)
    {
      CheckError(Emu->skip(samples));
    }
    
    void SetSoundFreq(uint_t soundFreq)
    {
      if (soundFreq != SoundFreq)
      {
        Reload(soundFreq);
      }
    }
    
  private:
    void Load(uint_t soundFreq)
    {
      auto emu = Tune->CreateEmu();
      CheckError(emu->set_sample_rate(soundFreq));
      CheckError(emu->load_mem(Tune->Data.data(), Tune->Data.size()));
      Emu.swap(emu);
      SoundFreq = soundFreq;
    }

    void Reload(uint_t soundFreq)
    {
      const auto oldPos = Emu ? Emu->tell() : 0;
      Load(soundFreq);
      Reset();
      if (oldPos)
      {
        CheckError(Emu->seek(oldPos));
      }
    }
  private:
    const GMETune::Ptr Tune;
    uint_t SoundFreq;
    EmuPtr Emu;
  };
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(GMETune::Ptr tune, StateIterator::Ptr iterator, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Iterator(std::move(iterator))
      , Analyzer(CreateSoundAnalyzer())
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
      , Engine(std::move(tune))
      , Target(std::move(target))
      , Looped()
    {
      ApplyParameters();
    }

    State::Ptr GetState() const override
    {
      return Iterator->GetStateObserver();
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

        auto data = Engine.Render(SoundParams->SamplesPerFrame());
        Analyzer->AddSoundData(data);
        Target->ApplyData(std::move(data));

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
      try
      {
        SoundParams.Reset();
        Iterator->Reset();
        Engine.Reset();
        Looped = {};
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

    void SetPosition(uint_t frame) override
    {
      try
      {
        SeekTune(frame);
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
      Module::SeekIterator(*Iterator, frame);
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
        Engine.SetSoundFreq(SoundParams->SoundFreq());
      }
    }

    void SeekTune(uint_t frame)
    {
      uint_t current = GetState()->Frame();
      if (frame < current)
      {
        Engine.Reset();
        current = 0;
      }
      if (const uint_t delta = frame - current)
      {
        Engine.Skip(delta * SoundParams->SamplesPerFrame());
      }
    }
  private:
    const StateIterator::Ptr Iterator;
    const SoundAnalyzer::Ptr Analyzer;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    GME Engine;
    const Sound::Receiver::Ptr Target;
    Sound::LoopParameters Looped;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(GMETune::Ptr tune, Information::Ptr info, Parameters::Accessor::Ptr props)
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
      try
      {
        return MakePtr<Renderer>(Tune, Module::CreateStreamStateIterator(Info), target, params);
      }
      catch (const std::exception& e)
      {
        throw Error(THIS_LINE, e.what());
      }
    }
  private:
    const GMETune::Ptr Tune;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  //TODO: rework, extract GYM parsing code to Formats library
  Dump DefaultDataCreator(Binary::View data)
  {
    return Dump(static_cast<const uint8_t*>(data.Start()), static_cast<const uint8_t*>(data.Start()) + data.Size());
  }
  
  using PlatformDetector = String (*)(Binary::View);
  
  struct PluginDescription
  {
    const Char* const Id;
    const uint_t ChiptuneCaps;
    const EmuCreator CreateEmu;
    const decltype(&DefaultDataCreator) CreateData;
    const PlatformDetector DetectPlatform;
  };
  
  namespace GYM
  {
    Dump CreateData(Binary::View data)
    {
      Binary::DataInputStream input(data);
      Binary::DataBuilder output(data.Size());
      const std::size_t packedSizeOffset = 424;
      output.Add(input.ReadData(packedSizeOffset));
      if (const auto packedSize = input.ReadLE<uint32_t>())
      {
        output.Add(uint32_t(0));
        Binary::Compression::Zlib::Decompress(input, output);
      }
      else
      {
        output.Add(packedSize);
        output.Add(input.ReadRestData());
      }
      Dump result;
      output.CaptureResult(result);
      return result;
    }
  }
 
  const auto PERIOD = Time::Milliseconds(20);
  
  Time::Milliseconds GetProperties(::track_info_t info, PropertiesHelper& props)
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
    props.SetFramesFrequency(PERIOD.ToFrequency());

    if (info.length > 0)
    {
      return Time::Milliseconds(info.length);
    }
    else if (info.loop_length > 0)
    {
      return Time::Milliseconds(info.intro_length + info.loop_length);
    }
    else
    {
      return Time::Milliseconds();
    }
  }
  
  class MultitrackFactory : public Module::Factory
  {
  public:
    MultitrackFactory(PluginDescription desc, Formats::Multitrack::Decoder::Ptr decoder)
      : Desc(std::move(desc))
      , Decoder(std::move(decoder))
    {
    }
    
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        if (const Formats::Multitrack::Container::Ptr container = Decoder->Decode(rawData))
        {
          if (container->TracksCount() > 1)
          {
            Require(HasContainer(Desc.Id, properties));
          }

          PropertiesHelper props(*properties);
          auto data = Desc.CreateData(*container);
          props.SetPlatform(Desc.DetectPlatform(data));
          const GMETune::Ptr tune = MakePtr<GMETune>(Desc.CreateEmu, std::move(data), container->StartTrackIndex());
          const auto storedDuration = GetProperties(tune->GetInfo(), props);
          const auto duration = storedDuration == Time::Milliseconds(0) ? Time::Milliseconds(GetDuration(params)) : storedDuration;
          const Information::Ptr info = CreateStreamInfo(duration.Divide<uint_t>(PERIOD));
        
          props.SetSource(*Formats::Chiptune::CreateMultitrackChiptuneContainer(container));
        
          return MakePtr<Holder>(tune, info, properties);
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create %1%: %2%", Desc.Id, e.what());
      }
      return Module::Holder::Ptr();
    }
  private:
    static bool HasContainer(const String& type, Parameters::Accessor::Ptr params)
    {
      Parameters::StringType container;
      Require(params->FindValue(Module::ATTR_CONTAINER, container));
      return container == type || boost::algorithm::ends_with(container, ">" + type);
    }
  private:
    const PluginDescription Desc;
    const Formats::Multitrack::Decoder::Ptr Decoder;
  };
  
  class SingletrackFactory : public Module::Factory
  {
  public:
    SingletrackFactory(PluginDescription desc, Formats::Chiptune::Decoder::Ptr decoder)
      : Desc(std::move(desc))
      , Decoder(std::move(decoder))
    {
    }
    
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        if (const Formats::Chiptune::Container::Ptr container = Decoder->Decode(rawData))
        {
          PropertiesHelper props(*properties);
          auto data = Desc.CreateData(*container);
          props.SetPlatform(Desc.DetectPlatform(data));
          const GMETune::Ptr tune = MakePtr<GMETune>(Desc.CreateEmu, std::move(data), 0);
          const auto storedDuration = GetProperties(tune->GetInfo(), props);
          const auto duration = storedDuration == Time::Milliseconds(0) ? Time::Milliseconds(GetDuration(params)) : storedDuration;
          const Information::Ptr info = CreateStreamInfo(duration.Divide<uint_t>(PERIOD));
        
          props.SetSource(*container);
        
          return MakePtr<Holder>(tune, info, properties);
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create %1%: %2%", Desc.Id, e.what());
      }
      return Module::Holder::Ptr();
    }
  private:
    const PluginDescription Desc;
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };

  struct MultitrackPluginDescription
  {
    typedef Formats::Multitrack::Decoder::Ptr (*MultitrackDecoderCreator)();
    typedef Formats::Chiptune::Decoder::Ptr (*ChiptuneDecoderCreator)(Formats::Multitrack::Decoder::Ptr);

    PluginDescription Desc;
    const MultitrackDecoderCreator CreateMultitrackDecoder;
    const ChiptuneDecoderCreator CreateChiptuneDecoder;
  };
  
  const MultitrackPluginDescription MULTITRACK_PLUGINS[] =
  {
    //nsf
    {
      {
        "NSF",
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::RP2A0X,
        &Create< ::Nsf_Emu>,
        &DefaultDataCreator,
        [](Binary::View) -> String {return Platforms::NINTENDO_ENTERTAINMENT_SYSTEM;}
      },
      &Formats::Multitrack::CreateNSFDecoder,
      &Formats::Chiptune::CreateNSFDecoder,
    },
    //nsfe
    {
      {
        "NSFE",
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::RP2A0X,
        &Create< ::Nsfe_Emu>,
        &DefaultDataCreator,
        [](Binary::View) -> String {return Platforms::NINTENDO_ENTERTAINMENT_SYSTEM;}
      },
      &Formats::Multitrack::CreateNSFEDecoder,
      &Formats::Chiptune::CreateNSFEDecoder,
    },
    //gbs
    {
      {
        "GBS",
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::LR35902,
        &Create< ::Gbs_Emu>,
        &DefaultDataCreator,
        [](Binary::View) -> String {return Platforms::GAME_BOY;}
      },
      &Formats::Multitrack::CreateGBSDecoder,
      &Formats::Chiptune::CreateGBSDecoder,
    },
    //kssx
    {
      {
        "KSSX",
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::MULTI,
        &Create< ::Kss_Emu>,
        &DefaultDataCreator,
        &KSS::DetectPlatform
      },
      &Formats::Multitrack::CreateKSSXDecoder,
      &Formats::Chiptune::CreateKSSXDecoder
    },
    //hes
    {
      {
        "HES",
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::HUC6270,
        &Create< ::Hes_Emu>,
        &DefaultDataCreator,
        [](Binary::View) -> String {return Platforms::PC_ENGINE;}
      },
      &Formats::Multitrack::CreateHESDecoder,
      &Formats::Chiptune::CreateHESDecoder
    },
  };
  
  struct SingletrackPluginDescription
  {
    typedef Formats::Chiptune::Decoder::Ptr (*ChiptuneDecoderCreator)();

    PluginDescription Desc;
    const ChiptuneDecoderCreator CreateChiptuneDecoder;
  };

  const SingletrackPluginDescription SINGLETRACK_PLUGINS[] =
  {
    //gym
    {
      {
        "GYM",
        ZXTune::Capabilities::Module::Type::STREAM | ZXTune::Capabilities::Module::Device::MULTI,
        &Create< ::Gym_Emu>,
        &GYM::CreateData,
        [](Binary::View) -> String {return Platforms::SEGA_GENESIS;}
      },
      &Formats::Chiptune::CreateGYMDecoder
    },
    //kss
    {
      {
        "KSS",
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::MULTI,
        &Create< ::Kss_Emu>,
        &DefaultDataCreator,
        &KSS::DetectPlatform
      },
      &Formats::Chiptune::CreateKSSDecoder
    }
  };
}
}

namespace ZXTune
{
  void RegisterMultitrackGMEPlugins(PlayerPluginsRegistrator& registrator)
  {
    for (const auto& desc : Module::GME::MULTITRACK_PLUGINS)
    {
      const Formats::Multitrack::Decoder::Ptr multi = desc.CreateMultitrackDecoder();
      const Formats::Chiptune::Decoder::Ptr decoder = desc.CreateChiptuneDecoder(multi);
      const Module::Factory::Ptr factory = MakePtr<Module::GME::MultitrackFactory>(desc.Desc, multi);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(desc.Desc.Id, desc.Desc.ChiptuneCaps, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
  
  void RegisterSingletrackGMEPlugins(PlayerPluginsRegistrator& registrator)
  {
    for (const auto& desc : Module::GME::SINGLETRACK_PLUGINS)
    {
      const Formats::Chiptune::Decoder::Ptr decoder = desc.CreateChiptuneDecoder();
      const Module::Factory::Ptr factory = MakePtr<Module::GME::SingletrackFactory>(desc.Desc, decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(desc.Desc.Id, desc.Desc.ChiptuneCaps, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }

  void RegisterGMEPlugins(PlayerPluginsRegistrator& registrator)
  {
    RegisterMultitrackGMEPlugins(registrator);
    RegisterSingletrackGMEPlugins(registrator);
  }
}

#undef FILE_TAG
