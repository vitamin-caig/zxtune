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

#include "formats/chiptune/digital/abysshighestexperience.h"
#include "module/players/platforms.h"
#include "module/players/properties_meta.h"

#include "binary/container_factories.h"
#include "debug/log.h"
#include "module/information.h"

#include "contract.h"
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

  Module::Information MakeTrackInformation(Binary::View data)
  {
    const auto hvl = LoadModule(data);
    uint_t loopTime = 0;
    while (!hvl->ht_SongEndReached)
    {
      hvl_NextFrame(hvl.get());
      if (hvl->ht_PosNr == hvl->ht_Restart && 0 == hvl->ht_NoteNr && hvl->ht_Tempo == hvl->ht_StepWaitFrames)
      {
        loopTime = hvl->ht_PlayingTime;
      }
    }
    const auto toTime = [&hvl](auto time) { return FRAME_DURATION * (time / hvl->ht_SpeedMultiplier); };
    Module::TrackLayout track{
        .ChannelsCount = hvl->ht_Channels, .PositionsCount = hvl->ht_PositionNr, .LoopPosition = hvl->ht_PosJump};
    return {.Duration = toTime(hvl->ht_PlayingTime),
            .LoopDuration = toTime(hvl->ht_PlayingTime - loopTime),
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
      const auto total = (FRAME_DURATION * (Hvl->ht_PlayingTime / Hvl->ht_SpeedMultiplier)).CastTo<Time::Millisecond>();
      uint_t channels = 0;
      for (uint_t idx = 0, lim = Hvl->ht_Channels; idx != lim; ++idx)
      {
        channels += Hvl->ht_Voices[idx].vc_TrackOn != 0;
      }
      return {.At = Time::AtMillisecond() + total,
              .Total = total,
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
    explicit Renderer(HVL::Ptr tune)
      : Tune(std::move(tune))
    {}

    State GetState() const override
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

    Module::Information GetModuleInformation() const override
    {
      return MakeTrackInformation(*Tune);
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
      return {};
    }

  private:
    const Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr Decoder;
  };

  Factory::Ptr CreateFactory(Formats::Chiptune::AbyssHighestExperience::Decoder::Ptr decoder)
  {
    return MakePtr<Factory>(std::move(decoder));
  }
}  // namespace Module::AHX
