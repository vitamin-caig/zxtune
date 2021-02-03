/**
* 
* @file
*
* @brief  libopenmpt support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
#include <core/core_parameters.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/container.h>
#include <module/track_information.h>
#include <module/track_state.h>
#include <module/players/analyzer.h>
#include <module/players/properties_helper.h>
#include <parameters/tracking_helper.h>
#include <sound/loop.h>
#include <strings/trim.h>
#include <time/duration.h>
//std includes
#include <utility>
//boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
//3rdparty includes
#define BUILDING_STATIC
#include <3rdparty/openmpt/libopenmpt/libopenmpt.hpp>

namespace Module
{
namespace Mpt
{
  const Debug::Stream Dbg("Core::OpenMPT");

  using ModulePtr = std::shared_ptr<openmpt::module>;

  Time::Milliseconds ToDuration(double seconds)
  {
    return Time::Milliseconds(seconds * Time::Milliseconds::PER_SECOND);
  }

  class Information : public Module::TrackInformation
  {
  public:
    typedef std::shared_ptr<const Information> Ptr;

    Information(ModulePtr track)
      : Track(std::move(track))
    {
    }

    Time::Milliseconds Duration() const override
    {
      return ToDuration(Track->get_duration_seconds());
    }

    Time::Milliseconds LoopDuration() const override
    {
      return Duration();//TODO
    }

    uint_t PositionsCount() const override
    {
      return Track->get_num_orders();
    }

    uint_t LoopPosition() const override
    {
      return 0; //TODO
    }

    uint_t ChannelsCount() const override
    {
      return Track->get_num_channels();
    }
  private:
    const ModulePtr Track;
  };

  class TrackState : public Module::TrackState
  {
  public:
    using Ptr = std::shared_ptr<TrackState>;

    explicit TrackState(ModulePtr track)
      : Track(std::move(track))
    {
    }

    Time::AtMillisecond At() const override
    {
      return Time::AtMillisecond() + ToDuration(Track->get_position_seconds() - LastLoopStart);
    }

    Time::Milliseconds Total() const override
    {
      return ToDuration(Track->get_position_seconds());
    }

    uint_t LoopCount() const override
    {
      return LoopsDone;
    }

    uint_t Position() const override
    {
      return Track->get_current_order();
    }

    uint_t Pattern() const override
    {
      return Track->get_current_pattern();
    }

    uint_t Line() const override
    {
      return Track->get_current_row();
    }

    uint_t Tempo() const override
    {
      return Track->get_current_tempo();
    }

    uint_t Quirk() const override
    {
      return 0;
    }

    uint_t Channels() const override
    {
      return Track->get_current_playing_channels();
    }

    void Looped()
    {
      LastLoopStart = Track->get_position_seconds();
      ++LoopsDone;
    }

    void Reset()
    {
      LastLoopStart = {};
      LoopsDone = 0;
    }
  private:
    const ModulePtr Track;
    double LastLoopStart = 0;
    uint_t LoopsDone = 0;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModulePtr track, uint_t samplerate, Parameters::Accessor::Ptr params)
      : Track(std::move(track))
      , State(MakePtr<TrackState>(Track))
      , Params(std::move(params))
      , Analyzer(Module::CreateSoundAnalyzer())
      , SoundFreq(samplerate)
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
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      static_assert(sizeof(Sound::Sample) == 4, "Incompatible sound sample size");

      ApplyParameters();
      const auto samples = SoundFreq / 10;//TODO
      Sound::Chunk chunk(samples);
      while (State->LoopCount() == 0 || looped(State->LoopCount()))
      {
        const auto done = Track->read_interleaved_stereo(SoundFreq, samples, safe_ptr_cast<int16_t*>(chunk.data()));
        if (done != samples)
        {
          State->Looped();
        }
        if (done)
        {
          chunk.resize(done);
          Analyzer->AddSoundData(chunk);
          return chunk;
        }
      }
      return {};
    }

    void Reset() override
    {
      Params.Reset();
      SetPosition({});
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      Track->set_position_seconds(double(request.Get()) / request.PER_SECOND);
      State->Reset();
    }
  private:
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        Parameters::IntType val = Parameters::ZXTune::Core::DAC::INTERPOLATION_DEFAULT;
        Params->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, val);
        const int interpolation = val != Parameters::ZXTune::Core::DAC::INTERPOLATION_NO ? 4 : 0;
        Track->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, interpolation);
      }
    }
  private:
    const ModulePtr Track;
    const TrackState::Ptr State;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    const Module::SoundAnalyzer::Ptr Analyzer;
    const uint_t SoundFreq;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModulePtr track, Parameters::Accessor::Ptr props)
      : Track(std::move(track))
      , Info(MakePtr<Information>(Track))
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

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      Require(!!Track);//TODO
      return MakePtr<Renderer>(std::move(Track), samplerate, std::move(params));
    }
  private:
    ModulePtr Track;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  struct PluginDescription
  {
    const char* const Id;
    const char* const Format;
    const char* const Description;
  };

  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    explicit Decoder(const PluginDescription& desc)
      : Desc(desc)
      , Fmt(Binary::CreateMatchOnlyFormat(Desc.Format))
    {
    }

    String GetDescription() const override
    {
      return Desc.Description;
    }


    Binary::Format::Ptr GetFormat() const override
    {
      return Fmt;
    }

    bool Check(Binary::View rawData) const override
    {
      return Fmt->Match(rawData);
    }

    Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const override
    {
      return Formats::Chiptune::Container::Ptr();//TODO
    }
  private:
    const PluginDescription& Desc;
    const Binary::Format::Ptr Fmt;
  };
  
  String DecodeString(String str)
  {
    const auto out = Strings::TrimSpaces(str);
    return out == str ? str : out.to_string();
  }

  class LogStub : public std::ostream
  {
  public:
    LogStub()
    {
      setstate(std::ios_base::badbit);
    }
  };

  static LogStub LOG;

  void FillMetadata(const openmpt::module& module, PropertiesHelper& props)
  {
    props.SetTitle(DecodeString(module.get_metadata("title")));
    props.SetAuthor(DecodeString(module.get_metadata("artist")));
    const auto tracker = DecodeString(module.get_metadata("tracker"));
    if (!tracker.empty())
    {
      props.SetProgram(tracker);
    }
    else
    {
      props.SetProgram(DecodeString(module.get_metadata("type_long")));
    }
    props.SetDate(DecodeString(module.get_metadata("date")));
    props.SetComment(DecodeString(module.get_metadata("message_raw")));
    {
      Strings::Array strings;
      using namespace boost::algorithm;
      split(strings, module.get_metadata("message_heuristic"), is_any_of("\r\n"), token_compress_on);
      if (!strings.empty())
      {
        props.SetStrings(strings);
      }
    }
  }
  
  class Factory : public Module::Factory
  {
  public:
    explicit Factory(const PluginDescription& desc)
      : Desc(desc)
    {
      Controls.emplace("play.at_end", "continue");
    }

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        //TODO: specify type filter
        auto track = ModulePtr(new openmpt::module(static_cast<const uint8_t*>(rawData.Start()), rawData.Size(), LOG, Controls));

        PropertiesHelper props(*properties);
        FillMetadata(*track, props);

        const auto moduleSize = rawData.Size();//TODO
        if (auto data = rawData.GetSubcontainer(0, moduleSize))
        {
          const auto source = Formats::Chiptune::CreateCalculatingCrcContainer(std::move(data), 0, moduleSize);
          props.SetSource(*source);
          return MakePtr<Holder>(std::move(track), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create OpenMPT module: %1%", e.what());
      }
      return {};
    }
  private:
    const PluginDescription& Desc;
    std::map<std::string, std::string> Controls;
  };

  const PluginDescription PLUGINS[] =
  {
    {
      "AMS"
      ,
      "'E'x't'r'e'm'e"
      "?"  // versionLow
      "01" // versionHigh
      ,
      "Extreme's Tracker"
      //, "AMS"
    },
    {
      "AMS"
      ,
      "'A'M'S'h'd'r 1a"
      ,
      "Velvet Studio"
      //, "AMS2"
    },
    {
      "DSM"
      ,
      // "RIFF ???? DSMF SONG"
      // no examples for second variant with "DSMF 0000/RIFF ????" 
      "'R'I'F'F ???? 'D'S'M'F"
      // song header should be located here
      "'S'O'N'G" //id
      "????"     //size
      "?{28}"    // title
      "??"       // version
      "??"       // flags
      "??"       // order pos
      "00-80 00" // restart pos <= 128
      "00-80 00" // num orders <= 128
      "??"       // num samples
      "? 00-01"  // num patterns <= 256
      "00-10 00" // num channels <= 16
      ,
      "Digital Sound Interface Kit RIFF"
      //, "DSM"
    },
    {
      "MT2"
      ,
      "'M'T'2'0"  // signature
      "????"      // userID
      "? 02"      // version 0x200..0x2ff
      "?{32}"     // trackeName
      "?{64}"     // songName
      "? 00-01"   // numOrders <= 256
      "??"        // restartPos
      "??"        // numPatterns
      "01-40 00"  // numChannels
      ,
      "Mad Tracker 2.xx"
      //, "MT2"
    },
    // ITP not supported due to external files
    {
      "PLM"
      ,
      "'P'L'M 1a" // signature
      "60-ff"     // header size 96+
      "10"        // version
      "?{48}"     // song name
      "01-20"     // num channels 1..32
      ,
      "Disorder Tracker 2"
      //, "PLM"
    },
    {
      "J2B"
      ,
      "'R'I'F'F ????"
      "'A'M 'F|'  'F|' "
      ,
      "Galaxy Sound System"
      //, "AM"
    },
    {
      "J2B"
      ,
      "'M'U'S'E"
      "de ad be|ba af|be"
      ,
      "Galaxy Sound System (compressed)"
      //, "J2B"
    },
    {
      "STP"
      ,
      "'S'T'P'3"
      "00 00-02" // be version
      "01-80"    // orders 1..128
      "? ?{128}" // patterns length, order list
      "?? ??"    // speed, speed frac
      "?? ??"    // timer count, flags
      "????"     // reserved
      "00 32"    // midi count == 50
      ,
      "Soundtracker Pro II"
      //, "STP"
    },
    {
      "C67"
      ,
      "01-0f"  // speed 1..15
      "?"      // restart pos
      "(?{12} 00){32}" // samples names
      "("
       "00000000" // unknown = 0 
       "? ? 0000" // length < 0x10000
       "???? ????" // loops
      "){32}" // samples
      "(?{12} 00){32}" // instrument names
      "("
       "0x ? ? ? ? %000000xx ? ? ? ? %000000xx"
      "){32}" // instruments
      ,
      "Composer 670"
      //, "C67"
    },
    {
      "MO3"
      ,
      "'M'O'3" // signature
      "00-05"  // version
      ,
      "Un4seen MO3"
      //, "MO3"
    },
  };
}
}

namespace ZXTune
{
  void RegisterMPTPlugins(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;
    for (const auto& desc : Module::Mpt::PLUGINS)
    {
      auto decoder = MakePtr<Module::Mpt::Decoder>(desc);
      auto factory = MakePtr<Module::Mpt::Factory>(desc);
      auto plugin = CreatePlayerPlugin(desc.Id, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}
