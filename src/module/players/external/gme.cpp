/**
 *
 * @file
 *
 * @brief  Game Music Emu-based formats support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/external/gme.h"

#include "module/players/duration.h"
#include "module/players/external/gym.h"
#include "module/players/external/kss.h"
#include "module/players/platforms.h"
#include "module/players/properties_helper.h"
#include "module/players/streaming.h"

#include "core/core_parameters.h"
#include "debug/log.h"
#include "formats/multitrack/container.h"
#include "math/numeric.h"
#include "module/attributes.h"
#include "module/holder.h"
#include "module/renderer.h"
#include "parameters/tracking_helper.h"
#include "strings/optimize.h"
#include "tools/xrange.h"

#include "contract.h"
#include "error.h"
#include "make_ptr.h"
#include "string_view.h"

#include "3rdparty/gme/gme/Gbs_Emu.h"
#include "3rdparty/gme/gme/Gme_File.h"
#include "3rdparty/gme/gme/Gym_Emu.h"
#include "3rdparty/gme/gme/Hes_Emu.h"
#include "3rdparty/gme/gme/Kss_Emu.h"
#include "3rdparty/gme/gme/Nsf_Emu.h"
#include "3rdparty/gme/gme/Nsfe_Emu.h"

namespace Module::GME
{
  const Debug::Stream Dbg("Module::GME");

  using EmuPtr = std::unique_ptr< ::Music_Emu>;

  inline void CheckError(::blargg_err_t err)
  {
    if (err)
    {
      throw std::runtime_error(err);
    }
  }

  using TimeBase = Time::Millisecond;

  using DataCreator = Binary::Data::Ptr (*)(const Binary::Container&);
  using PlatformDetector = StringView (*)(Binary::View);

  struct TuneInfo : ::track_info_t
  {
    Strings::Array Channels;
  };

  struct GMETune
  {
    using Ptr = std::shared_ptr<GMETune>;

    GMETune(::gme_type_t type, Binary::Data::Ptr data, uint_t track)
      : Type(type)
      , Data(std::move(data))
      , Track(track)
    {}

    const ::gme_type_t Type;
    const Binary::Data::Ptr Data;
    const uint_t Track;
    Time::Milliseconds Duration;

    TuneInfo GetInfo() const
    {
      const EmuPtr emu(Type->new_info());
      CheckError(emu->load_mem(Data->Start(), Data->Size()));
      TuneInfo info;
      CheckError(emu->track_info(&info, Track));
      if (auto chans = emu->voice_count())
      {
        info.Channels.resize(chans);
        for (auto i : xrange(chans))
        {
          info.Channels[i] = emu->voice_name(i);
        }
      }
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
      : Emu(tune.Type->new_emu())
      , SoundFreq(samplerate)
      , Track(tune.Track)
    {
      // TODO: effects_buffer
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

    void SetChannelsMask(int mask)
    {
      Emu->mute_voices(mask);
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
    Renderer(GMETune::Ptr tune, uint_t samplerate, Parameters::Accessor::Ptr params)
      : Tune(std::move(tune))
      , State(Tune->Duration)
      , Params(std::move(params))
      , Engine(*Tune, samplerate)
    {}

    Module::State GetState() const override
    {
      return State.Get();
    }

    Sound::Chunk Render() override
    {
      ApplyParameters();
      const auto avail = State.ConsumeUpTo(FRAME_DURATION);
      return Engine.Render(GetSamples(avail));
    }

    void Reset() override
    {
      try
      {
        State.Reset();
        ResetEngine();
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
    void ResetEngine()
    {
      Engine.Reset();
      Params.Reset();
    }

    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Core;
        const auto val = Parameters::GetInteger(*Params, CHANNELS_MASK, CHANNELS_MASK_DEFAULT);
        Engine.SetChannelsMask(val);
      }
    }

    uint_t GetSamples(Time::Microseconds period) const
    {
      return period.Get() * Engine.GetSoundFreq() / period.PER_SECOND;
    }

    void SeekTune(Time::AtMillisecond request)
    {
      if (request < State.At())
      {
        ResetEngine();
      }
      if (const auto toSkip = State.Seek(request))
      {
        Engine.Skip(GetSamples(toSkip));
      }
    }

  private:
    const GMETune::Ptr Tune;
    TimedState State;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    GME Engine;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(GMETune::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {}

    Information GetModuleInformation() const override
    {
      return CreateTimedInfo(Tune->Duration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      try
      {
        return MakePtr<Renderer>(Tune, samplerate, std::move(params));
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

  Binary::Data::Ptr DefaultDataCreator(const Binary::Container& data)
  {
    return data.GetSubcontainer(0, data.Size());
  }

  void GetProperties(const TuneInfo& info, PropertiesHelper& props)
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
    props.SetChannels(info.Channels);
  }

  class MultitrackFactory : public Module::MultitrackFactory
  {
  public:
    MultitrackFactory(::gme_type_t type, PlatformDetector detectPlatform)
      : Type(type)
      , DetectPlatform(detectPlatform)
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& params, const Formats::Multitrack::Container& container,
                             Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        auto data = DefaultDataCreator(container);
        props.SetPlatform(DetectPlatform(*data));
        auto tune = MakePtr<GMETune>(Type, std::move(data), container.StartTrackIndex());

        const auto info = tune->GetInfo();
        GetProperties(info, props);
        tune->SetDuration(info, params);

        return MakePtr<Holder>(std::move(tune), std::move(properties));
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create {}: {}", Type->extension_, e.what());
      }
      return {};
    }

  private:
    const ::gme_type_t Type;
    const PlatformDetector DetectPlatform;
  };

  class SingletrackFactory : public ExternalParsingFactory
  {
  public:
    SingletrackFactory(::gme_type_t type, DataCreator createData, PlatformDetector detectPlatform)
      : Type(type)
      , CreateData(createData)
      , DetectPlatform(detectPlatform)
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& params, const Formats::Chiptune::Container& container,
                             Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        auto data = CreateData(container);
        props.SetPlatform(DetectPlatform(*data));
        auto tune = MakePtr<GMETune>(Type, std::move(data), 0);
        const auto info = tune->GetInfo();
        GetProperties(info, props);
        tune->SetDuration(info, params);

        return MakePtr<Holder>(std::move(tune), std::move(properties));
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create {}: {}", Type->extension_, e.what());
      }
      catch (const Error& e)
      {
        Dbg("Failed to create {}: {}", Type->extension_, e.ToString());
      }
      return {};
    }

  private:
    const ::gme_type_t Type;
    const DataCreator CreateData;
    const PlatformDetector DetectPlatform;
  };

  MultitrackFactory::Ptr CreateNsfFactory()
  {
    return MakePtr<MultitrackFactory>(
        ::Nsf_Emu::static_type(), [](Binary::View) -> StringView { return Platforms::NINTENDO_ENTERTAINMENT_SYSTEM; });
  }

  MultitrackFactory::Ptr CreateNsfeFactory()
  {
    return MakePtr<MultitrackFactory>(
        ::Nsfe_Emu::static_type(), [](Binary::View) -> StringView { return Platforms::NINTENDO_ENTERTAINMENT_SYSTEM; });
  }

  MultitrackFactory::Ptr CreateGbsFactory()
  {
    return MakePtr<MultitrackFactory>(::Gbs_Emu::static_type(),
                                      [](Binary::View) -> StringView { return Platforms::GAME_BOY; });
  }

  MultitrackFactory::Ptr CreateKssxFactory()
  {
    return MakePtr<MultitrackFactory>(::Kss_Emu::static_type(), &KSS::DetectPlatform);
  }

  MultitrackFactory::Ptr CreateHesFactory()
  {
    return MakePtr<MultitrackFactory>(::Hes_Emu::static_type(),
                                      [](Binary::View) -> StringView { return Platforms::PC_ENGINE; });
  }

  ExternalParsingFactory::Ptr CreateGymFactory()
  {
    return MakePtr<SingletrackFactory>(::Gym_Emu::static_type(), &GYM::CreateData,
                                       [](Binary::View) -> StringView { return Platforms::SEGA_GENESIS; });
  }

  ExternalParsingFactory::Ptr CreateKssFactory()
  {
    return MakePtr<SingletrackFactory>(::Kss_Emu::static_type(), &DefaultDataCreator, &KSS::DetectPlatform);
  }
}  // namespace Module::GME
