/**
 *
 * @file
 *
 * @brief  libopenmpt support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/container.h"
#include "module/players/properties_helper.h"

#include "binary/format_factories.h"
#include "core/core_parameters.h"
#include "core/plugin_attrs.h"
#include "debug/log.h"
#include "math/numeric.h"
#include "parameters/tracking_helper.h"
#include "strings/format.h"
#include "strings/sanitize.h"
#include "strings/split.h"
#include "time/duration.h"
#include "tools/xrange.h"

#include "contract.h"
#include "make_ptr.h"
#include "string_view.h"

#include "3rdparty/openmpt/libopenmpt/libopenmpt_ext.hpp"

#include <memory>
#include <utility>

namespace Module::Mpt
{
  const Debug::Stream Dbg("Core::OpenMPT");

  using ModulePtr = std::shared_ptr<openmpt::module_ext>;

  Time::Milliseconds ToDuration(double seconds)
  {
    return Time::Milliseconds{static_cast<uint_t>(seconds * Time::Milliseconds::PER_SECOND)};
  }

  // TODO: implement proper loop-related calculations after https://bugs.openmpt.org/view.php?id=1675 fix
  Information MakeInformation(const openmpt::module_ext& mod)
  {
    TrackLayout track = {.ChannelsCount = static_cast<uint_t>(mod.get_num_channels()),
                         .PositionsCount = static_cast<uint_t>(mod.get_num_orders()),
                         .LoopPosition = 0 /*TODO*/};
    const auto duration = ToDuration(mod.get_duration_seconds());
    return {.Duration = duration, .LoopDuration = duration /*TODO*/, .Track = std::move(track)};
  }

  std::vector<double> GetPositionPoints(openmpt::module& track)
  {
    const auto positions = track.get_num_orders();
    std::vector<double> result(positions);
    for (std::int32_t pos = 0; pos < positions; ++pos)
    {
      result[pos] = track.set_position_order_row(pos, 0);
      Dbg("pos[{}] = {:.2f}s", pos, result[pos]);
    }
    track.set_position_order_row(0, 0);
    return result;
  }

  class TrackState
  {
  public:
    explicit TrackState(openmpt::module_ext& track)
      : Track(track)
      , TotalDuration(Track.get_duration_seconds())
      , Positions(GetPositionPoints(track))
    {
      Reset();
    }

    Time::AtMillisecond At() const
    {
      return Time::AtMillisecond() + ToDuration(std::min(TotalDuration, Current.Time - AllLoopsDuration));
    }

    Time::Milliseconds Total() const
    {
      return ToDuration(Current.Time);
    }

    Module::State Get() const
    {
      return {.At = At(),
              .Total = Total(),
              .LoopCount = LoopsDone,
              .Track = {{.Position = Current.Position,
                         .Pattern = static_cast<uint_t>(Track.get_current_pattern()),
                         .Line = static_cast<uint_t>(Track.get_current_row()),
                         .Tempo = static_cast<uint_t>(Track.get_current_tempo()),
                         .Channels = static_cast<uint_t>(Track.get_current_playing_channels())}}};
    }

    void Update()
    {
      Current.Position = static_cast<uint_t>(Track.get_current_order());
      Current.Time = Track.get_position_seconds();
      if (Current.Time - AllLoopsDuration > TotalDuration)
      {
        ++LoopsDone;
        AllLoopsDuration = Current.Time - Positions[Current.Position];
        Dbg("Detected loop to {} at {:.2f}s, {} total loops done ({:.2f}s)", Current.Position,
            Positions[Current.Position], LoopsDone, AllLoopsDuration);
      }
    }

    void ForcedLoop()
    {
      ++LoopsDone;
      Dbg("Forced loop, {} total", LoopsDone);
    }

    void Reset()
    {
      LoopsDone = 0;
      AllLoopsDuration = 0.0;
      Update();
    }

  private:
    const openmpt::module_ext& Track;
    const double TotalDuration;
    const std::vector<double> Positions;
    uint_t LoopsDone = 0;
    double AllLoopsDuration = 0.0;

    struct PositionIndex
    {
      uint_t Position = 0;
      double Time;
    };
    PositionIndex Current = {};
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(ModulePtr track, uint_t samplerate, Parameters::Accessor::Ptr params)
      : Track(std::move(track))
      , InteractiveTrack(*static_cast<openmpt::ext::interactive*>(Track->get_interface(openmpt::ext::interactive_id)))
      , State(*Track)
      , Params(std::move(params))
      , SoundFreq(samplerate)
    {}

    Module::State GetState() const override
    {
      return State.Get();
    }

    Sound::Chunk Render() override
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      static_assert(sizeof(Sound::Sample) == 4, "Incompatible sound sample size");

      ApplyParameters();
      const auto samples = SoundFreq / 10;  // TODO
      Sound::Chunk chunk(samples);
      for (;;)
      {
        State.Update();
        if (const auto done = Track->read_interleaved_stereo(SoundFreq, samples, safe_ptr_cast<int16_t*>(chunk.data())))
        {
          chunk.resize(done);
          return chunk;
        }
        else if (State.Total())
        {
          // see XM.cheapchoon%20II%20%20%203-30.gz @ AMP
          Track->set_position_seconds(0);
          State.ForcedLoop();
        }
        else
        {
          break;
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
      State.Reset();
    }

  private:
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Core;
        const auto val = Parameters::GetInteger(*Params, DAC::INTERPOLATION, DAC::INTERPOLATION_DEFAULT);
        // cubic interpolation vs windowed sinc with 8 taps
        const int interpolation = val != DAC::INTERPOLATION_NO ? 8 : 3;
        Track->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, interpolation);
        ApplyMuting(Parameters::GetInteger(*Params, CHANNELS_MASK, CHANNELS_MASK_DEFAULT));
      }
    }

    void ApplyMuting(uint_t newMask)
    {
      for (uint_t chan = 0, diff = MuteMask ^ newMask; diff != 0; ++chan, diff >>= 1)
      {
        if (diff & 1)
        {
          InteractiveTrack.set_channel_mute_status(chan, newMask & (1 << chan));
        }
      }
      MuteMask = newMask;
    }

  private:
    const ModulePtr Track;
    openmpt::ext::interactive& InteractiveTrack;
    TrackState State;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    const uint_t SoundFreq;
    uint_t MuteMask = 0;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModulePtr track, Parameters::Accessor::Ptr props)
      : Track(std::move(track))
      , Properties(std::move(props))
    {}

    Module::Information GetModuleInformation() const override
    {
      return MakeInformation(*Track);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      Require(!!Track);  // TODO
      return MakePtr<Renderer>(Track, samplerate, std::move(params));
    }

  private:
    ModulePtr Track;
    const Parameters::Accessor::Ptr Properties;
  };

  struct PluginDescription
  {
    const ZXTune::PluginId Id;
    const StringView Format;
    const StringView Description;
  };

  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    explicit Decoder(const PluginDescription& desc)
      : Desc(desc)
      , Fmt(Binary::CreateMatchOnlyFormat(Desc.Format))
    {}

    StringView GetDescription() const override
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

    Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (Check(rawData))
      {
        return Formats::Chiptune::CreateCalculatingCrcContainer(rawData);
      }
      return {};
    }

  private:
    const PluginDescription& Desc;
    const Binary::Format::Ptr Fmt;
  };

  void FillMetadata(StringView type, openmpt::module_ext& module, PropertiesHelper& props)
  {
    props.SetTitle(Strings::Sanitize(module.get_metadata("title")));
    props.SetAuthor(Strings::Sanitize(module.get_metadata("artist")));
    const auto tracker = Strings::Sanitize(module.get_metadata("tracker"));
    if (!tracker.empty())
    {
      props.SetProgram(tracker);
    }
    else
    {
      props.SetProgram(Strings::Sanitize(module.get_metadata("type_long")));
    }
    props.SetDate(Strings::Sanitize(module.get_metadata("date")));
    props.SetComment(Strings::SanitizeMultiline(module.get_metadata("message_raw")));
    {
      const auto metadata = module.get_metadata("message_heuristic");
      if (const auto splitted = Strings::Split(metadata, "\r\n"sv); !splitted.empty())
      {
        std::vector<String> strings(splitted.size());
        std::transform(splitted.begin(), splitted.end(), strings.begin(), &Strings::SanitizeKeepPadding);
        props.SetStrings(strings);
      }
    }
    if (module.get_interface(openmpt::ext::interactive_id))
    {
      const auto chans = module.get_num_channels();
      auto names = module.get_channel_names();
      names.resize(chans);
      for (auto i : xrange(chans))
      {
        if (names[i].empty())
        {
          names[i] = Strings::Format("{} {}"sv, type, i + 1);
        }
      }
      props.SetChannels(names);
    }
  }

  const double MIN_DURATION = 0.1;
  const double MAX_DURATION = 3600;  // 1 hour

  class Factory : public Module::ExternalParsingFactory
  {
  public:
    explicit Factory(const PluginDescription& desc)
      : Desc(desc)
    {
      // In conjunction with set_repeat_count(-1) disables zero rendered samples in general
      Controls.emplace("play.at_end", "continue");
    }

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/,
                                     const Formats::Chiptune::Container& container,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        // TODO: specify type filter
        auto track = std::make_shared<openmpt::module_ext>(container.Start(), container.Size(), nullptr, Controls);

        // play all subsongs
        track->select_subsong(-1);

        // use external repeats control
        track->set_repeat_count(-1);

        if (!track->get_num_orders() || !Math::InRange(track->get_duration_seconds(), MIN_DURATION, MAX_DURATION))
        {
          return {};
        }

        PropertiesHelper props(*properties);
        FillMetadata(Desc.Id, *track, props);

        return MakePtr<Holder>(std::move(track), std::move(properties));
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create OpenMPT module: {}", e.what());
      }
      return {};
    }

  private:
    const PluginDescription& Desc;
    std::map<std::string, std::string> Controls;
  };

  using ZXTune::operator""_id;

  // clang-format off
  const PluginDescription PLUGINS[] =
  {
    {
      "XM"_id
      ,
      "'E'x't'e'n'd'e'd' 'M'o'd'u'l'e':' "sv
      ,
      "FastTracker II"sv
      //, "XM"
    },
    {
      "IT"_id
      ,
      "'I | 't"
      "'M | 'p"
      "'P | 'm"
      "'M | '."
      ""sv
      ,
      "Impulse Tracker"sv
      //, "IT"
    },
    {
      "S3M"_id
      ,
      "?{28}"    // title
      "?"        // eof
      "10"       // file type
      "??"       // reserved
      "?{10}"    // sizes, flags
      "01|02 00" // version
      "'S'C'R'M"
      ""sv
      ,
      "ScreamTracker 3"sv
      //, "S3M"
    },
    {
      "STM"_id
      ,
      "?{20}"    // songname
      "20-7e{8}" // trackername
      "02|1a"    // eof
      "02"       // type=module
      "02"       // major
      "00|0a|14|15" // minor
      "?"        // tempo
      "01-40"    // num patterns
      "00-40|58" // global volume or placeholder
      ""sv
      ,
      "ScreamTracker 2"sv
      //, STM
    },
    {
      "MED"_id
      ,
      "'M'M'D '0-'3" // signature
      ""sv
      ,
      "OctaMED / MED Soundstudio"sv
      //, MED
    },
    {
      "MTM"_id
      ,
      "'M'T'M" // signature
      "00-1f"  // version
      "?{20}"  // songName
      "??"     // numTracks
      "?"      // lastPattern
      "00-7f"  // lastOrder
      "??"     // commentSize
      "? ?"    // numSamples, attribute
      "00-40"  // beatsPerTrack
      "01-20"  // numChannels
      ""sv
      ,
      "MultiTracker"sv
      //, MTM
    },
    {
      "MDL"_id
      ,
      "'D'M'D'L" // signature
      "00-1f"    // version
      ""sv
      ,
      "Digitrakker"sv
      //, "MDL"
    },
    {
      "DBM"_id
      ,
      "'D'B'M'0" // signagure
      "00-03"    // trkVerHi
      ""sv
      ,
      "DigiBooster Pro"sv
      //, "DBM"
    },
    {
      "FAR"_id
      ,
      "'F'A'R fe"  // signature
      "?{40}"      // songName
      "0d0a1a"     // eof
      ""sv
      ,
      "Farandole Composer"sv
      //, "FAR"
    },
    {
      "AMS"_id
      ,
      "'E'x't'r'e'm'e"
      "?"  // versionLow
      "01" // versionHigh
      ""sv
      ,
      "Extreme's Tracker"sv
      //, "AMS"
    },
    {
      "AMS"_id
      ,
      "'A'M'S'h'd'r 1a"sv
      ,
      "Velvet Studio"sv
      //, "AMS2"
    },
    {
      "OKT"_id
      ,
      "'O'K'T'A'S'O'N'G" // signature
      "(20-7f){4}"  // iff id
      ""sv
      ,
      "Oktalyzer"sv
      //, "OKT"
    },
    {
      "PTM"_id
      ,
      "?{28}"    // songname
      "1a"       // dosEOF
      "? 00-02"  // version
      "?"        // reserved
      "? 00-01"  // numOrders 0..256
      "01-ff 00" // numSamples 1..255
      "01-80 00" // numPatterns 1..128
      "01-20 00" // numChannels 1..32
      "00 00 ??" // flags, reserved
      "'P'T'M'F" // magic
      ""sv
      ,
      "PolyTracker"sv
      //, "PTM"
    },
    {
      "ULT"_id
      ,
      "'M'A'S'_'U'T'r'a'c'k'_'V'0'0"
      "'1-'4"
      ""sv
      ,
      "UltraTracker"sv
      //, "ULT"
    },
    {
      "DMF"_id
      ,
      "'D'D'M'F"  // signature
      "01-0a"     // version
      ""sv
      ,
      "X-Tracker"sv
      //, "DMF"
    },
    {
      "DSM"_id
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
      ""sv
      ,
      "Digital Sound Interface Kit RIFF"sv
      //, "DSM"
    },
    {
      "AMF"_id
      ,
      "'A'S'Y'L'U'M' 'M'u's'i'c' 'F'o'r'm'a't' 'V'1'.'0 00" // signature
      "? ?"   // speed, tempo
      "01-3f" // numSamples
      ""sv
      ,
      "ASYLUM Music Format"sv
      //, "AMF_Asylum"
    },
    {
      "AMF"_id
      ,
      "'A'M'F"  // signature
      "08-0e"   // version
      "?{32}"   // title
      "? ? ?"   // samples, orders, tracks
      "00-20"   // channels
      ""sv
      ,
      "DSMI / Digital Sound And Music Interface"sv
      //, "AMF_DSMI"
    },
    {
      "PSM"_id
      ,
      "'P'S'M' " // signature
      "????"     // fileSize
      "'F'I'L'E" // fileInfoID
      ""sv
      ,
      "Epic MegaGames MASI"sv
      //, "PSM"
    },
    {
      "PSM"_id
      ,
      "'P'S'M fe"  // formatID
      "?{59}"      // songTitle
      "1a"         // lineEnd
      "%xxxxxx00"  // songType
      "01|10"      // formatVersion
      "00"         // patternVersion
      "? ? ?"      // speed, tempo, masterVolume
      "?? ?? ?? ??" // length, orders, patterns, samples
      "? 00-01"    // channelsPlay
      "? 00-01"    // channelsReal
      ""sv
      ,
      "Epic MegaGames MASI (Old Version)"sv
      //, "PSM16"
    },
    {
      "MT2"_id
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
      ""sv
      ,
      "Mad Tracker 2.xx"sv
      //, "MT2"
    },
    // ITP not supported due to external files
    {
      "GDM"_id
      ,
      "'G'D'M fe"  // magic
      "?{32}"      // songTitle
      "?{32}"      // musician
      "0d 0a 1a"   // dosEOF
      "'G'M'F'S"   // magic2
      "01 00"      // format major, minor
      "?? ? ?"     // trackerId, tracker major, minor
      "?{32}"      // pan map
      "? ? ?"      // master vol, tempo, bpm
      "01-09 00"   // originalFormat
      ""sv
      ,
      "BWSB Soundsystem"sv
      //, "GDM"
    },
    {
      "IMF"_id
      ,
      "?{32}"   // title
      "? 00-01" // ordNum
      "??"      // patNum
      "? 00-01" // insNum 256?
      "?? ?{8}" // flags, unused
      "? ? ? ?" // tempo, bpm, master, amp
      "?{8}"    // unused2
      "'I'M'1'0" // signature
      "(?{15} 00-02){32}"  // channels
      ""sv
      ,
      "Imago Orpheus"sv
      //, "IMF"
    },
    {
      "DBM"_id
      ,
      "'D'I'G'I' 'B'o'o's't'e'r' 'm'o'd'u'l'e 00"
      "?{4} ?" // version + int
      "01-08"  // numChannels
      ""sv
      ,
      "Digi Booster"sv
      //, "DIGI"
    },
    {
      "DTM"_id
      ,
      "'D'.'T'." // magic
      "00 00 00 0e-ff" // headerSize
      "00"             // type
      ""sv
      ,
      "Digital Tracker / Digital Home Studio"sv
      //, "DTM"
    },
    {
      "PLM"_id
      ,
      "'P'L'M 1a" // signature
      "60-ff"     // header size 96+
      "10"        // version
      "?{48}"     // song name
      "01-20"     // num channels 1..32
      ""sv
      ,
      "Disorder Tracker 2"sv
      //, "PLM"
    },
    {
      "J2B"_id
      ,
      "'R'I'F'F ????"
      "'A'M 'F|'  'F|' "
      ""sv
      ,
      "Galaxy Sound System"sv
      //, "AM"
    },
    {
      "MOD"_id
      ,
      "'F'O'R'M"
      "????"
      "'M'O'D'L"
      ""sv
      ,
      "ProTracker 3.6"sv
      //, "PT36"
    },
    // no examples for MUS_KM
    {
      "FMT"_id
      ,
      "'F'M'T'r'a'c'k'e'r 01 01" // magic
      "?{20}" // trackerName
      "?{32}" // songName
      "(?{8} ?{8} %000000xx{3}){8}" // channels
      ""sv
      ,
      "Davey W Taylor's FM Tracker"sv
      //, "FMT"
    },
    {
      "SFX"_id
      ,
      "(00 00-02 ?? ){15}" // samples offsets up to 131072 BE
      "'S 'O 'N 'G" // magic
      ""sv
      ,
      "SoundFX 1.x"sv
      //, "SFX"
    },
    {
      "SFX"_id
      ,
      "(00 00-02 ?? ){31}" // samples offsets up to 131072 BE
      "'S 'O '3 '1" // magic
      ""sv
      ,
      "SoundFX 2.0 / MultiMedia Sound"sv
      //, "SFX"
    },
    {
      "STP"_id
      ,
      "'S'T'P'3"
      "00 00-02" // be version
      "01-80"    // orders 1..128
      "? ?{128}" // patterns length, order list
      "?? ??"    // speed, speed frac
      "?? ??"    // timer count, flags
      "????"     // reserved
      "00 32"    // midi count == 50
      ""sv
      ,
      "Soundtracker Pro II"sv
      //, "STP"
    },
    {
      "MOD"_id
      ,
      "?{1080}" // skip
      "('M      |'P|'N|'L|'F|'N|'O   |'C   |'M|'8|'F   |'F|'E|'1-'9|'0-'9|'T)"
      "('.|'!|'&|'A|'S|'A|'E|'.|'C|'K|'D   |00   |'A   |'L|'X|'C   |'0-'9|'D)"
      "('K      |'T|'M|'R|'S|'T|'T   |'8|'6|00   |'0   |'T|'O|'H   |'C   |'Z)"
      "('.|'!   |'T|'S|'D|'T|'.|'A   |'1   |00   |'4-'8|'4-'9|'N   |'H|'N|'4-'9)"
      ""sv
      ,
      "Generic MOD-compatible"sv
      //, "MOD"
    },
    {
      "MOD"_id
      ,
      "?{1464}"
      "'M'T'N"
      "00"
      ""sv
      ,
      "MnemoTroN SoundTracker (MOD-compatible)"sv
      //, "ICE"
    },
    {
      "MOD"_id
      ,
      "?{1464}"
      "'I'T'1'0"
      ""sv
      ,
      "Ice Tracker (MOD-compatible)"sv
      //, "ICE"
    },
    {
      "669"_id
      ,
      "'i|'J 'f|'N" // magic
      "?{108}"      // message
      "01-40"       // samples
      "01-80"       // patterns
      "00-7f"       // restart pos
      "(00-7f|fe|ff){128}" // orders
      "?{128}"      // tempoList
      "(00-3f){128}"// breaks
      ""sv
      ,
      "669 Composer / UNIS 669"sv
      //, "669"
    },
    {
      "C67"_id
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
      ""sv
      ,
      "Composer 670"sv
      //, "C67"
    },
    {
      "MO3"_id
      ,
      "'M'O'3" // signature
      "00-05"  // version
      ""sv
      ,
      "Un4seen MO3"sv
      //, "MO3"
    },
    {
      "MOD"_id
      ,
      "(00|08|20-7f){20}"  //name
      "("                  //instruments
       "?{22}"             // name
       "00-7f?"            // BE size
       "00"                // finetune
       "00-40"             // volume
       "??"                // BE loop start
       "00-7f?"            // BE loop size
      "){15}"
      "00-80"           //len
      "00-dc"           //restart
      "(00-3f){128}"    //order
      ""sv
      ,
      "Ultimate Soundtracker / etc (MOD Compatible)"sv
      //, "M15"
    },
    {
      "DSYM"_id
      ,
      "020113131412010b"  //magic
      "00-01"             //version
      "01-08"             //channels
      "? 00-10"           //le orders up to 4096
      "? 00-10"           //le tracks up to 4096
      ""sv
      ,
      "Digital Symphony"sv
      //, "DSYM"
    },
    {
      "SYMMOD"_id
      ,
      "'S'y'm'M"          //magic
      "00000001"          //be version==1
      ""sv
      ,
      "Symphonie"sv
      //, "SYMMOD"
    },
    {
      "667"_id
      ,
      "'g'f"              //magic
      "(20-ff{8}){64}"    //instruments
      "01-0f"             //speed
      "00-80"             //orders
      ""sv
      ,
      "Composer 667"sv
    },
    {
      "CBA"_id
      ,
      "'C'B'A f9"         //magic
      "?{32}"             //title
      "1a"                //eof
      "??"                //messagelength
      "01-20"             //numChannels
      "? ? ?"             //lastPattern, numOrders, numSamples
      "01-ff"             //speed
      "20-ff"             //tempo
      ""sv
      ,
      "Chuck Biscuits / Black Artist"sv
    },
    {
      "ETX"_id
      ,
      "'E'A'S'Y'T'R'A'X' '1'.'0 01 00" //magic
      "01-ff"                          //tempo
      "00-1f"                          //lastPattern
      "(??00-7f00){4}"                 //le offsets up to 0x800000
      ""sv
      ,
      "EasyTrax"sv
    },
    {
      "FC"_id
      ,
      "'S|'F"                  //magic
      "'M|'C"
      "'O|'1"
      "'D|'4"
      "????"                   //sequenceSize
      "("
      "00 00-08 ??"            //be offset up to 0x80000
      "0000 00-40 %xx000000"   //be size up to 0x4000, multiple of 0x40
      "){3}"                   //
      "(00 00-08 ? ?){2}"       //be offsets up to 0x80000
      ""sv
      ,
      "Future Composer 1.0 - 1.4"sv
    },
    {
      "FTM"_id
      ,
      "'F'T'M'N"              //magic
      "03"                    //version
      "00-3f"                 //numSamples
      "??"                    //numMeasures
      "10-4f?"                //be tempo 0x1000..0x4fff
      "00-0b"                 //tonality
      "?"                     //muteStatus
      "00-3f"                 //globalVolume
      "%000000xx"             //flags
      "01-18"                 //tickperrow
      "04-60"                 //rowspermeasure
      "?{64}"                 //title+author
      "00-40"                 //numEffects
      "00"                    //padding
      ""sv
      ,
      "Face The Music"sv
    },
    {
      "GMC"_id
      ,
      "("
      "00 00-1f ? %xxxxxxx0"  //be offset 0..0x1fffff, even
      "00-7f ?"               //be length up to 0x7fff
      "00"                    //zero
      "00-40"                 //volume
      "00 00-1f ? %xxxxxxx0"  //be address 0..0x1fffff, even
      "??"                    //loop length
      "00-7f ?"               //be data start up to 0x7fff
      "){15}"                 //samples
      "00 00 00"              //zeroes
      "01-64"                 //orders count 1..100
      "(%xxxxxx00 00){100}"   //orders, be multiple of 1024
      ""sv
      ,
      "Game Music Creator"sv
    },
    {
      "GTK"_id
      ,
      "'G'T'K"                //magic
      "01-04"                 //version
      "?{32}?{160}"           //name+comment
      "00 ?"                  //be samples up to 0xff
      "00-01 ?"               //be rows up to 0x100
      "00 01-20"              //be channels 1..0x20
      "00-01 ?"               //be orders up to 0x100
      ""sv
      ,
      "Graoumf Tracker 1/2"sv
    },
    {
      "IMS"_id
      ,
      "(00 | 20-ff){20}"       // song name
      "("
      "?{22}"                  //name
      "00-80 ?"                //be length
      "00"                     //finetune
      "?"                      //volume
      "?? ??"                  //loop start+length
      "){31}"                  //samples
      "01-80"                  //numOrders
      ""sv
      ,
      "Images Music System"sv
    },
    {
      "KRIS"_id
      ,
      "?{952}"
      "'K'R'I'S"              //magic
      "00-80"                 //numOrders
      "00-7f"                 //restartPos
      ""sv
      ,
      "ChipTracker"sv
    },
    /*{
      "NRU"_id
      ,
      "("
      "00 00-40"              //be volume
      "00 00-1f ? %xxxxxxx0"  //be addr up to 0x1fffff, even
      "00-7f ?"               //be length up to 0x7fff
      "?{4} ?{2} ?{2}"        //be loop addr,len, finetune
      "){31}"
      "?{454}"
      ""sv
      ,
      "NoiseRunner"sv
    },*/
    {
      "PUMA"_id
      ,
      "(00 | 20-ff){12}"      //songName
      "00 ?"                  //be lastOrder up to 0xff
      "00 01-80"              //be numPatterns 1..0x80
      "00 01-1f"              //be numInstruments 1..0x1f
      "00 00"                 //unknown
      "(00 00-10 ? ?){10}"    //be sampleOffset up to 0x10'0000
      ""sv
      ,
      "PumaTracker"sv
    },
    {
      "RTM"_id
      ,
      "'R'T'M'M"           //magic
      "20"                 //space
      "?{32}"              //name
      "1a"                 //eof
      "00-12 01"           //le version 0x100..0x112
      ""sv
      ,
      "Real Tracker 2"sv
    },
    /*{
      "SS"_id
      ,
      "'S|'I|'I"
      "'O|'A|'A"
      "'N|'N|'N"
      "'G|'9|'9"
      "'O|'O|'2"
      "'K|'K|'a"          //magic
      "03-ff %xx000000"   //be multiple of 64, from 14*64
      "00 00-1f"          //be speed up to 15
      "? 00-80"           //be numOrders with lo word up to 128
      ""sv
      ,
      "Apple IIgs SoundSmith / MegaTracker"sv
    },*/ // only w/external samples
    {
      "TCB"_id
      ,
      "'A'N' 'C'O'O'L '.|'!"  //magic
      "00 00 00 00-7f"        //be numPatterns up to 127
      "00-0e"                 //tempo
      "00"                    //unused
      "(00-7f){128}"          //orders
      "00-7f"                 //numOrders
      "00"                    //unused
      ""sv
      ,
      "TCB Tracker"sv
    },
  };
  // clang-format on
}  // namespace Module::Mpt

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
}  // namespace ZXTune
