/**
 *
 * @file
 *
 * @brief  AHX chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/dac/abysshighestexperience.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/container_factories.h>
#include <debug/log.h>
#include <formats/chiptune/digital/abysshighestexperience.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/track_information.h>
#include <module/track_state.h>
// 3rdparty includes
#include <3rdparty/hvl/hvl_replay.h>

namespace Module::AHX
{
  const Debug::Stream Dbg("Core::AHXSupp");

  typedef std::shared_ptr<hvl_tune> HvlPtr;

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
    const HvlPtr result =
        HvlPtr(hvl_ParseTune(static_cast<const uint8*>(data.Start()), data.Size(), samplerate, MONO), &hvl_FreeTune);
    Require(result.get() != nullptr);
    return result;
  }

  const auto FRAME_DURATION = Time::Milliseconds::FromFrequency(50);

  // TODO: test this
  class TrackInformation : public Module::TrackInformation
  {
  public:
    explicit TrackInformation(Binary::View data)
      : Hvl(LoadModule(data))
    {}

    Time::Milliseconds Duration() const override
    {
      Initialize();
      return (FRAME_DURATION * (Hvl->ht_PlayingTime / Hvl->ht_SpeedMultiplier));
    }

    Time::Milliseconds LoopDuration() const override
    {
      Initialize();
      return (FRAME_DURATION * ((Hvl->ht_PlayingTime - LoopTime) / Hvl->ht_SpeedMultiplier));
    }

    uint_t PositionsCount() const override
    {
      return Hvl->ht_PositionNr;
    }

    uint_t LoopPosition() const override
    {
      return Hvl->ht_PosJump;
    }

    uint_t ChannelsCount() const override
    {
      return Hvl->ht_Channels;
    }

    uint_t GetLoopTime() const
    {
      Initialize();
      return LoopTime;
    }

  private:
    void Initialize() const
    {
      while (!Hvl->ht_SongEndReached)
      {
        hvl_NextFrame(Hvl.get());
        if (Hvl->ht_PosNr == Hvl->ht_Restart && 0 == Hvl->ht_NoteNr && Hvl->ht_Tempo == Hvl->ht_StepWaitFrames)
        {
          LoopTime = Hvl->ht_PlayingTime;
        }
      }
    }

  private:
    const HvlPtr Hvl;
    mutable uint_t LoopTime = 0;
  };

  class TrackState : public Module::TrackState
  {
  public:
    TrackState(HvlPtr hvl)
      : Hvl(std::move(hvl))
    {}

    Time::AtMillisecond At() const override
    {
      if (Hvl->ht_SongEndReached)
      {
        // TODO: investigate
        return Time::AtMillisecond() + Total();
      }
      else
      {
        return Time::AtMillisecond() + Total();
      }
    }

    Time::Milliseconds Total() const override
    {
      return (FRAME_DURATION * (Hvl->ht_PlayingTime / Hvl->ht_SpeedMultiplier)).CastTo<Time::Millisecond>();
    }

    uint_t LoopCount() const override
    {
      return Hvl->ht_SongEndReached;
    }

    uint_t Position() const override
    {
      return Hvl->ht_PosNr;
    }

    uint_t Pattern() const override
    {
      return Hvl->ht_PosNr;  // TODO
    }

    uint_t Line() const override
    {
      return Hvl->ht_NoteNr;
    }

    uint_t Tempo() const override
    {
      return Hvl->ht_Tempo;
    }

    uint_t Quirk() const override
    {
      return Hvl->ht_Tempo - Hvl->ht_StepWaitFrames;
    }

    uint_t Channels() const override
    {
      uint_t result = 0;
      for (uint_t idx = 0, lim = Hvl->ht_Channels; idx != lim; ++idx)
      {
        result += Hvl->ht_Voices[idx].vc_TrackOn != 0;
      }
      return result;
    }

  private:
    const HvlPtr Hvl;
  };

  class HVL
  {
  public:
    typedef std::shared_ptr<HVL> Ptr;

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

    uint_t LoopCount() const
    {
      return Hvl->ht_SongEndReached;
    }

    TrackState::Ptr MakeTrackState() const
    {
      return MakePtr<TrackState>(Hvl);
    }

  private:
    const HvlPtr Hvl;
    uint_t SamplesPerFrame;
  };

  class Renderer : public Module::Renderer
  {
  public:
    explicit Renderer(HVL::Ptr tune)
      : Tune(std::move(tune))
    {}

    State::Ptr GetState() const override
    {
      return Tune->MakeTrackState();
    }

    Sound::Chunk Render() override
    {
      return Tune->RenderFrame();
    }

    void Reset() override
    {
      Tune->Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      Tune->Seek(request);
    }

  private:
    const HVL::Ptr Tune;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Binary::Data::Ptr tune, Parameters::Accessor::Ptr props)
      : Tune(std::move(tune))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      if (!Info)
      {
        Info = MakePtr<TrackInformation>(*Tune);
      }
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(MakePtr<HVL>(*Tune, samplerate));
    }

  private:
    const Binary::Data::Ptr Tune;
    const Parameters::Accessor::Ptr Properties;
    mutable Information::Ptr Info;
  };

  class DataBuilder : public Formats::Chiptune::AbyssHighestExperience::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

  private:
    MetaProperties Meta;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr decoder)
      : Decoder(std::move(decoder))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Decoder->Parse(rawData, dataBuilder))
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
      return Module::Holder::Ptr();
    }

  private:
    const Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr Decoder;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(std::move(decoder));
  }
}  // namespace Module::AHX
