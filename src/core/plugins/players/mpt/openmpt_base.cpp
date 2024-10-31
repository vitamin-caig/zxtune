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

#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/view.h"
#include "core/core_parameters.h"
#include "core/plugin_attrs.h"
#include "debug/log.h"
#include "formats/chiptune.h"
#include "math/numeric.h"
#include "module/holder.h"
#include "module/renderer.h"
#include "module/track_information.h"
#include "module/track_state.h"
#include "parameters/container.h"
#include "parameters/identifier.h"
#include "parameters/tracking_helper.h"
#include "parameters/types.h"
#include "sound/chunk.h"
#include "strings/sanitize.h"
#include "strings/split.h"
#include "time/duration.h"
#include "time/instant.h"

#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"
#include "types.h"

#include "3rdparty/openmpt/libopenmpt/libopenmpt.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace Module::Mpt
{
  const Debug::Stream Dbg("Core::OpenMPT");

  using ModulePtr = std::shared_ptr<openmpt::module>;

  Time::Milliseconds ToDuration(double seconds)
  {
    return Time::Milliseconds{static_cast<uint_t>(seconds * Time::Milliseconds::PER_SECOND)};
  }

  // TODO: implement proper loop-related calculations after https://bugs.openmpt.org/view.php?id=1675 fix
  class Information : public Module::TrackInformation
  {
  public:
    using Ptr = std::shared_ptr<const Information>;

    Information(ModulePtr track)
      : Track(std::move(track))
    {}

    Time::Milliseconds Duration() const override
    {
      return ToDuration(Track->get_duration_seconds());
    }

    Time::Milliseconds LoopDuration() const override
    {
      return Duration();  // TODO
    }

    uint_t PositionsCount() const override
    {
      return Track->get_num_orders();
    }

    uint_t LoopPosition() const override
    {
      return 0;  // TODO
    }

    uint_t ChannelsCount() const override
    {
      return Track->get_num_channels();
    }

  private:
    const ModulePtr Track;
  };

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

  class TrackState : public Module::TrackState
  {
  public:
    using Ptr = std::shared_ptr<TrackState>;

    explicit TrackState(ModulePtr track)
      : Track(std::move(track))
      , TotalDuration(Track->get_duration_seconds())
      , Positions(GetPositionPoints(*Track))
    {
      Reset();
    }

    Time::AtMillisecond At() const override
    {
      return Time::AtMillisecond() + ToDuration(std::min(TotalDuration, Current.Time - AllLoopsDuration));
    }

    Time::Milliseconds Total() const override
    {
      return ToDuration(Current.Time);
    }

    uint_t LoopCount() const override
    {
      return LoopsDone;
    }

    uint_t Position() const override
    {
      return Current.Position;
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

    void Update()
    {
      Current.Position = static_cast<uint_t>(Track->get_current_order());
      Current.Time = Track->get_position_seconds();
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
    const ModulePtr Track;
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
      , State(MakePtr<TrackState>(Track))
      , Params(std::move(params))
      , SoundFreq(samplerate)
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
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
        State->Update();
        if (const auto done = Track->read_interleaved_stereo(SoundFreq, samples, safe_ptr_cast<int16_t*>(chunk.data())))
        {
          chunk.resize(done);
          return chunk;
        }
        else if (State->Total())
        {
          // see XM.cheapchoon%20II%20%20%203-30.gz @ AMP
          Track->set_position_seconds(0);
          State->ForcedLoop();
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
      State->Reset();
    }

  private:
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Core::DAC;
        const auto val = Parameters::GetInteger(*Params, INTERPOLATION, INTERPOLATION_DEFAULT);
        // cubic interpolation vs windowed sinc with 8 taps
        const int interpolation = val != INTERPOLATION_NO ? 8 : 3;
        Track->set_render_param(openmpt::module::render_param::RENDER_INTERPOLATIONFILTER_LENGTH, interpolation);
      }
    }

  private:
    const ModulePtr Track;
    const TrackState::Ptr State;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    const uint_t SoundFreq;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(ModulePtr track, Parameters::Accessor::Ptr props)
      : Track(std::move(track))
      , Info(MakePtr<Information>(Track))
      , Properties(std::move(props))
    {}

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
      Require(!!Track);  // TODO
      return MakePtr<Renderer>(Track, samplerate, std::move(params));
    }

  private:
    ModulePtr Track;
    const Information::Ptr Info;
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
        const auto size = rawData.Size();
        auto data = rawData.GetSubcontainer(0, size);
        return Formats::Chiptune::CreateCalculatingCrcContainer(std::move(data), 0, size);
      }
      return {};
    }

  private:
    const PluginDescription& Desc;
    const Binary::Format::Ptr Fmt;
  };

  void FillMetadata(const openmpt::module& module, PropertiesHelper& props)
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
        auto track = std::make_shared<openmpt::module>(static_cast<const uint8_t*>(container.Start()), container.Size(),
                                                       nullptr, Controls);

        // play all subsongs
        track->select_subsong(-1);

        // use external repeats control
        track->set_repeat_count(-1);

        if (!track->get_num_orders() || !Math::InRange(track->get_duration_seconds(), MIN_DURATION, MAX_DURATION))
        {
          return {};
        }

        PropertiesHelper props(*properties);
        FillMetadata(*track, props);

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
    }
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
