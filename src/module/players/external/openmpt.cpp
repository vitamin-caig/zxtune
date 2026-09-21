/**
 *
 * @file
 *
 * @brief  libopenmpt-based formats support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/external/openmpt.h"

#include "module/players/properties_helper.h"

#include "core/core_parameters.h"
#include "debug/log.h"
#include "formats/chiptune/container.h"
#include "math/numeric.h"
#include "module/holder.h"
#include "module/renderer.h"
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

namespace Module::MPT
{
  const Debug::Stream Dbg("Module::MPT");

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

    Information GetModuleInformation() const override
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

  class Factory : public ExternalParsingFactory
  {
  public:
    explicit Factory(StringView id)
      : Id(id)
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
        FillMetadata(Id, *track, props);

        return MakePtr<Holder>(std::move(track), std::move(properties));
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create OpenMPT module: {}", e.what());
      }
      return {};
    }

  private:
    const String Id;
    std::map<std::string, std::string> Controls;
  };

  ExternalParsingFactory::Ptr CreateFactory(StringView id)
  {
    return MakePtr<Factory>(id);
  }
}  // namespace Module::MPT
