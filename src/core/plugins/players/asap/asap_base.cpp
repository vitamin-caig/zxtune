/**
* 
* @file
*
* @brief  ASAP-based formats support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
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
#include <sound/resampler.h>
#include <strings/optimize.h>
//boost includes
#include <boost/algorithm/string/predicate.hpp>
//3rdparty
#include <3rdparty/asap/asap.h>
//text includes
#include <module/text/platforms.h>

#define FILE_TAG 90B9A91A

namespace Module
{
namespace ASAP
{
  const Debug::Stream Dbg("Core::ASAPSupp");

  using TimeUnit = Time::Millisecond;
  
  class AsapTune
  {
  public:
    using Ptr = std::shared_ptr<AsapTune>;
  
    AsapTune(const String& id, Binary::View data, int track)
      : Module(::ASAP_New())
      , Info()
      , Track(track)
      , Channels()
    {
      CheckError(::ASAP_Load(Module, ("dummy." + id).c_str(), static_cast<const unsigned char*>(data.Start()), data.Size()), "ASAP_Load");
      Reset();//required for subsequential calls
      Info = ::ASAP_GetInfo(Module);
      Require(Track == ::ASAPInfo_GetDefaultSong(Info));
      Channels = ::ASAPInfo_GetChannels(Info);
      Dbg("Track %1%, %2% channels", Track, Channels);
      Require(Channels == 1 || Channels == 2);
    }
    
    ~AsapTune()
    {
      ::ASAP_Delete(Module);
    }
    
    int GetSongsCount() const
    {
      return ::ASAPInfo_GetSongs(Info);
    }

    void FillDuration(const Parameters::Accessor& params)
    {
      if (::ASAPInfo_GetDuration(Info, Track) <= 0)
      {
        const auto duration = GetDefaultDuration(params);
        ::ASAPInfo_SetDuration(const_cast<ASAPInfo*>(Info), Track, duration.CastTo<TimeUnit>().Get());
      }
    }

    Time::Duration<TimeUnit> GetDuration() const
    {
      return Time::Duration<TimeUnit>(::ASAPInfo_GetDuration(Info, Track));
    }
    
    void GetProperties(Binary::View data, PropertiesHelper& props) const
    {
      const auto title = Strings::OptimizeAscii(::ASAPInfo_GetTitle(Info));
      const auto author = Strings::OptimizeAscii(::ASAPInfo_GetAuthor(Info));
      const auto date = Strings::OptimizeAscii(::ASAPInfo_GetDate(Info));
      
      props.SetTitle(title);
      props.SetAuthor(author);
      props.SetDate(date);
      Strings::Array instruments;
      for (int idx = 0; ; ++idx)
      {
        if (const auto ins = ::ASAPInfo_GetInstrumentName(Info, static_cast<const unsigned char*>(data.Start()), data.Size(), idx))
        {
          instruments.push_back(Strings::OptimizeAscii(ins));
        }
        else
        {
          break;
        }
      }
      props.SetStrings(instruments);
    }
    
    void Reset()
    {
      CheckError(::ASAP_PlaySong(Module, Track, -1), "ASAP_PlaySong");
    }
    
    Sound::Chunk Render(uint_t samples)
    {
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      Sound::Chunk result(samples);
      const auto fmt = isLE() ? ASAPSampleFormat_S16_L_E : ASAPSampleFormat_S16_B_E;
      const auto stereo = result.data();
      if (Channels == 2)
      {
        const int bytes = samples * sizeof(*stereo);
        CheckError(bytes == ::ASAP_Generate(Module, safe_ptr_cast<unsigned char*>(stereo), bytes, fmt), "ASAP_Generate");
      }
      else
      {
        const auto mono = safe_ptr_cast<Sound::Sample::Type*>(stereo) + samples;
        const int bytes = samples * sizeof(*mono);
        CheckError(bytes == ::ASAP_Generate(Module, safe_ptr_cast<unsigned char*>(mono), bytes, fmt), "ASAP_Generate");
        std::transform(mono, mono + samples, stereo, [](Sound::Sample::Type val) {return Sound::Sample(val, val);});
      }
      return result;
    }
    
    void Seek(Time::Instant<TimeUnit> request)
    {
      CheckError(::ASAP_Seek(Module, request.Get()), "ASAP_Seek");
    }
  private:
    void CheckError(bool ok, const char* msg)
    {
      if (!ok)
      {
        throw Error(THIS_LINE, msg);
      }
    }
  private:
    struct ASAP* const Module;
    const struct ASAPInfo* Info;
    int Track;
    int Channels;
  };
  
  const auto FRAME_DURATION = Time::Milliseconds(100);

  uint_t GetSamples(Time::Microseconds period)
  {
    return period.Get() * ASAP_SAMPLE_RATE / period.PER_SECOND;
  }
  
  class Renderer : public Module::Renderer
  {
  public:
    Renderer(AsapTune::Ptr tune, Sound::Converter::Ptr target)
      : Tune(std::move(tune))
      , State(MakePtr<TimedState>(Tune->GetDuration()))
      , Analyzer(Module::CreateSoundAnalyzer())
      , Target(std::move(target))
    {
    }

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Analyzer;
    }

    Sound::Chunk Render(const Sound::LoopParameters& looped) override
    {
      if (!State->IsValid())
      {
        return {};
      }
      const auto avail = State->Consume(FRAME_DURATION, looped);

      auto buf = Target->Apply(Tune->Render(GetSamples(avail)));
      Analyzer->AddSoundData(buf);
      return buf;
    }

    void Reset() override
    {
      try
      {
        State->Reset();
        Tune->Reset();
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      State->Seek(request);
      try
      {
        Tune->Seek(State->At());
      }
      catch (const std::exception& e)
      {
        Dbg(e.what());
      }
    }
  private:
    const AsapTune::Ptr Tune;
    const TimedState::Ptr State;
    const Module::SoundAnalyzer::Ptr Analyzer;
    const Sound::Converter::Ptr Target;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(AsapTune::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {
    }

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->GetDuration());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      try
      {
        return MakePtr<Renderer>(Tune, Sound::CreateResampler(ASAP_SAMPLE_RATE, samplerate));
      }
      catch (const std::exception& e)
      {
        throw Error(THIS_LINE, e.what());
      }
    }
  private:
    const AsapTune::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
  };
  
  struct PluginDescription
  {
    const Char* const Id;
    const uint_t ChiptuneCaps;
  };
  
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
        if (const auto container = Decoder->Decode(rawData))
        {
          if (container->TracksCount() > 1)
          {
            Require(HasContainer(Desc.Id, properties));
          }

          auto tune = MakePtr<AsapTune>(Desc.Id, rawData, container->StartTrackIndex());
          
          PropertiesHelper props(*properties);
          props.SetPlatform(Platforms::ATARI);
          tune->GetProperties(rawData, props);
        
          props.SetSource(*Formats::Chiptune::CreateMultitrackChiptuneContainer(container));

          tune->FillDuration(params);
          return MakePtr<Holder>(std::move(tune), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create %1%: %2%", Desc.Id, e.what());
      }
      return {};
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
    {
      //sap
      {
        "SAP",
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::CO12294,
      },
      &Formats::Multitrack::CreateSAPDecoder,
      &Formats::Chiptune::CreateSAPDecoder,
    }
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
        if (const auto container = Decoder->Decode(rawData))
        {
          auto tune = MakePtr<AsapTune>(Desc.Id, rawData, 0);
          
          Require(tune->GetSongsCount() == 1);
          
          PropertiesHelper props(*properties);
          props.SetPlatform(Platforms::ATARI);
          tune->GetProperties(rawData, props);

          props.SetSource(*container);
        
          tune->FillDuration(params);
          return MakePtr<Holder>(std::move(tune), std::move(properties));
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
  
  struct SingletrackPluginDescription
  {
    typedef Formats::Chiptune::Decoder::Ptr (*ChiptuneDecoderCreator)();

    PluginDescription Desc;
    const ChiptuneDecoderCreator CreateChiptuneDecoder;
  };

  const SingletrackPluginDescription SINGLETRACK_PLUGINS[] =
  {
    //rmt
    {
      {
        "RMT",
        ZXTune::Capabilities::Module::Type::MEMORYDUMP | ZXTune::Capabilities::Module::Device::CO12294,
      },
      &Formats::Chiptune::CreateRasterMusicTrackerDecoder
    },
  };
}
}

namespace ZXTune
{
  void RegisterASAPPlugins(PlayerPluginsRegistrator& registrator)
  {
    for (const auto& desc : Module::ASAP::MULTITRACK_PLUGINS)
    {
      const Formats::Multitrack::Decoder::Ptr multi = desc.CreateMultitrackDecoder();
      const Formats::Chiptune::Decoder::Ptr decoder = desc.CreateChiptuneDecoder(multi);
      const Module::Factory::Ptr factory = MakePtr<Module::ASAP::MultitrackFactory>(desc.Desc, multi);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(desc.Desc.Id, desc.Desc.ChiptuneCaps, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
    for (const auto& desc : Module::ASAP::SINGLETRACK_PLUGINS)
    {
      const Formats::Chiptune::Decoder::Ptr decoder = desc.CreateChiptuneDecoder();
      const Module::Factory::Ptr factory = MakePtr<Module::ASAP::SingletrackFactory>(desc.Desc, decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(desc.Desc.Id, desc.Desc.ChiptuneCaps, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}

#undef FILE_TAG
