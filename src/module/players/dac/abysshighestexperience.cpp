/**
 *
 * @file
 *
 * @brief  AHX chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/dac/abysshighestexperience.h"

#include "core/plugins/parameters.h"
#include "formats/chiptune/digital/abysshighestexperience.h"
#include "module/players/generic_presentation.h"
#include "module/players/platforms.h"
#include "module/players/properties_meta.h"

#include "binary/container_factories.h"
#include "debug/log.h"
#include "module/information.h"
#include "module/track_presentation.h"
#include "parameters/tracking_helper.h"

#include "contract.h"
#include "lazy.h"
#include "make_ptr.h"
#include "pointers.h"

#include "3rdparty/hvl/hvl_replay.h"

namespace Module::AHX
{
  const Debug::Stream Dbg("Core::AHXSupp");

  using HvlPtr = std::shared_ptr<hvl_tune>;

  enum StereoSeparation
  {
    MONO = 0,
    STEREO = 4
  };

  HvlPtr LoadModule(Binary::View data, uint_t samplerate = 44100)
  {
    static bool initialized = false;
    if (!initialized)
    {
      hvl_InitReplayer();
      initialized = true;
    }
    auto result =
        HvlPtr(hvl_ParseTune(static_cast<const uint8*>(data.Start()), data.Size(), samplerate, MONO), &hvl_FreeTune);
    Require(result != nullptr);
    return result;
  }

  const auto FRAME_DURATION = Time::Milliseconds::FromFrequency(50);

  bool IsHively(Binary::View data)
  {
    const auto id = data.SubView(0, 3);
    const auto* const bytes = id.As<uint8_t>();
    return bytes && id.Size() == 3 && bytes[0] == 'H' && bytes[1] == 'V' && bytes[2] == 'L';
  }

  Module::Information MakeTrackInformation(hvl_tune& hvl)
  {
    hvl_InitSubsong(&hvl, 0);
    uint_t loopTime = 0;
    while (!hvl.ht_SongEndReached)
    {
      hvl_NextFrame(&hvl);
      if (hvl.ht_PosNr == hvl.ht_Restart && 0 == hvl.ht_NoteNr && hvl.ht_Tempo == hvl.ht_StepWaitFrames)
      {
        loopTime = hvl.ht_PlayingTime;
      }
    }
    const auto toTime = [&hvl](auto time) { return FRAME_DURATION * (time / hvl.ht_SpeedMultiplier); };
    Module::TrackLayout track{
        .ChannelsCount = hvl.ht_Channels, .PositionsCount = hvl.ht_PositionNr, .LoopPosition = hvl.ht_PosJump};
    return {.Duration = toTime(hvl.ht_PlayingTime),
            .LoopDuration = toTime(hvl.ht_PlayingTime - loopTime),
            .Track = std::move(track)};
  }

  class HVL
  {
  public:
    using Ptr = std::shared_ptr<HVL>;

    HVL(Binary::View data, uint_t samplerate)
      : Hvl(LoadModule(data, samplerate))
      , SamplesPerFrame(FRAME_DURATION.Get() * samplerate / FRAME_DURATION.PER_SECOND)
    {
      Reset();
    }

    void Reset()
    {
      hvl_InitSubsong(Hvl.get(), 0);
    }

    void SetChannelsMask(uint_t mask)
    {
      hvl_SetChannelsMask(Hvl.get(), mask);
    }

    Sound::Chunk RenderFrame()
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      Sound::Chunk result(SamplesPerFrame);
      auto* const buf = safe_ptr_cast<int8*>(result.data());
      hvl_DecodeFrame(Hvl.get(), buf, buf + sizeof(Sound::Sample) / 2, sizeof(Sound::Sample));
      return result;
    }

    void Seek(Time::AtMillisecond request)
    {
      const auto frame = Hvl->ht_SpeedMultiplier * request.Get() / FRAME_DURATION.Get();
      uint_t current = Hvl->ht_PlayingTime;
      if (frame < current)
      {
        Reset();
        current = 0;
      }
      for (; current < frame; ++current)
      {
        hvl_NextFrame(Hvl.get());
      }
    }

    Module::State MakeTrackState() const
    {
      const auto played =
          (FRAME_DURATION * (Hvl->ht_PlayingTime / Hvl->ht_SpeedMultiplier)).CastTo<Time::Millisecond>();
      uint_t channels = 0;
      for (uint_t idx = 0, lim = Hvl->ht_Channels; idx != lim; ++idx)
      {
        channels += Hvl->ht_Voices[idx].vc_TrackOn != 0;
      }
      return {.At = Time::AtMillisecond() + played,
              .LoopCount = Hvl->ht_SongEndReached,
              .Track = {{.Position = static_cast<uint_t>(Hvl->ht_PosNr),
                         .Pattern = static_cast<uint_t>(Hvl->ht_PosNr) /*TODO*/,
                         .Line = static_cast<uint_t>(Hvl->ht_NoteNr),
                         .Tempo = static_cast<uint_t>(Hvl->ht_Tempo),
                         .Quirk = static_cast<uint_t>(Hvl->ht_Tempo - Hvl->ht_StepWaitFrames),
                         .Channels = channels}}};
    }

  private:
    const HvlPtr Hvl;
    uint_t SamplesPerFrame;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(HVL::Ptr tune, Parameters::Accessor::Ptr params)
      : Tune(std::move(tune))
      , Params(std::move(params))
    {}

    State GetState() const override
    {
      return Tune->MakeTrackState();
    }

    Sound::Chunk Render() override
    {
      ApplyParameters();
      return Tune->RenderFrame();
    }

    void Reset() override
    {
      Params.Reset();
      Tune->Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      // Tune->Seek() internally calls hvl_InitSubsong on backward seeks, which clears ht_ChannelsMask
      Params.Reset();
      Tune->Seek(request);
    }

  private:
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Core;
        const auto val = Params.Get<ChannelsMask>();
        Tune->SetChannelsMask(val);
      }
    }

  private:
    const HVL::Ptr Tune;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
  };

  namespace Presentation
  {
    using Delimiter = Module::Presentation::Delimiter;
    using Layout = Module::Presentation::Layout<'0'>;

    struct AhxChannel
    {
      Delimiter Separator;
      Module::Presentation::NoteAndOctave Note;
      Layout::Hex<2> Instrument;
      Layout::Hex<1> Effect;
      Layout::Hex<2> Param;

      void Fill(const hvl_step& step)
      {
        if (step.stp_Note)
        {
          Note.Set(step.stp_Note - 1);
        }
        Instrument = step.stp_Instrument;
        Effect = step.stp_FX;
        Param = step.stp_FXParam;
      }
    };

    struct HvlChannel
    {
      Delimiter Separator;
      Module::Presentation::NoteAndOctave Note;
      char Space = ' ';
      Layout::Hex<2> Instrument;
      char Space2 = ' ';
      Layout::Hex<1> Effect;
      Layout::Hex<2> Param;
      char Space3 = ' ';
      Layout::Hex<1> Effect2;
      Layout::Hex<2> Param2;

      void Fill(const hvl_step& step)
      {
        if (step.stp_Note)
        {
          Note.Set(step.stp_Note - 1);
        }
        if (step.stp_Instrument)
        {
          Instrument = step.stp_Instrument;
        }
        else
        {
          Instrument.fill(' ');
        }
        Effect = step.stp_FX;
        Param = step.stp_FXParam;
        Effect2 = step.stp_FXb;
        Param2 = step.stp_FXbParam;
      }
    };

    struct Line
    {
      Module::Presentation::Dec<2> Index;
    };

    template<class Channel>
    String ForPattern(const hvl_tune& tune, uint_t index)
    {
      if (index >= tune.ht_PositionNr)
      {
        return {};
      }
      const auto& position = tune.ht_Positions[index];
      const auto lines = tune.ht_TrackLength;
      const auto channels = tune.ht_Channels;
      String result;
      result.reserve(static_cast<std::size_t>(lines) * (1 + sizeof(Line) + std::size_t(channels) * sizeof(Channel)));
      for (uint_t row = 0; row != lines; ++row)
      {
        auto& dst = Module::Presentation::AppendLine<Line>(result);
        dst.Index = row;
        for (uint_t channel = 0; channel != channels; ++channel)
        {
          auto& cell = Module::Presentation::AppendTo<Channel>(result);
          cell.Fill(tune.ht_Tracks[position.pos_Track[channel]][row]);
        }
      }
      return result;
    }
  }  // namespace Presentation

  class Holder
    : public Module::Holder
    , public TrackPresentation
  {
  public:
    using Ptr = Module::Holder::Ptr;

    explicit Holder(Binary::Data::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Hively(IsHively(*Tune))
      , PatternData([this] { return LoadModule(*Tune); })
      , Properties(std::move(props))
    {}

    Module::Information GetModuleInformation() const override
    {
      return MakeTrackInformation(*PatternData);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      return MakePtr<Renderer>(MakePtr<HVL>(*Tune, samplerate), std::move(params));
    }

    String GetPattern(uint_t index) const override
    {
      const auto& tune = *PatternData;
      return Hively ? Presentation::ForPattern<Presentation::HvlChannel>(tune, index)
                    : Presentation::ForPattern<Presentation::AhxChannel>(tune, index);
    }

  private:
    const Binary::Data::Ptr Tune;
    const bool Hively;
    const Lazy<HvlPtr> PatternData;
    const Parameters::Accessor::Ptr Properties;
  };

  class DataBuilder : public Formats::Chiptune::AbyssHighestExperience::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Props(props)
      , Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetChannels(uint_t count) override
    {
      Strings::Array chans;
      for (uint_t ch = 0; ch < count; ++ch)
      {
        chans.emplace_back(1, 'A' + ch);
      }
      Props.SetChannels(chans);
    }

  private:
    PropertiesHelper& Props;
    MetaProperties Meta;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::AbyssHighestExperience::Parser parse)
      : Parse(std::move(parse))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Parse(rawData, dataBuilder))
        {
          props.SetSource(*container);
          props.SetPlatform(Platforms::AMIGA);

          auto tune = Binary::CreateContainer(Binary::View(*container));
          return MakePtr<Holder>(std::move(tune), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create AHX: {}", e.what());
      }
      return {};
    }

  private:
    const Formats::Chiptune::AbyssHighestExperience::Parser Parse;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::AbyssHighestExperience::Parser parse)
  {
    return MakePtr<Factory>(std::move(parse));
  }
}  // namespace Module::AHX
