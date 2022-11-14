/**
 *
 * @file
 *
 * @brief  V2M chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/dac/v2m.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <formats/chiptune/digital/v2m.h>
#include <module/players/platforms.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <sound/resampler.h>
// 3rdparty includes
#include <3rdparty/v2m/src/sounddef.h>
#include <3rdparty/v2m/src/v2mconv.h>
#include <3rdparty/v2m/src/v2mplayer.h>
// std includes
#include <algorithm>

namespace Module::V2M
{
  const Debug::Stream Dbg("Core::V2MSupp");

  using DataPtr = std::shared_ptr<const uint8_t>;

  // Different versions of stdlib has different traits...
  void Deleter(uint8_t* data)
  {
    delete[] data;
  }

  struct V2mSoundDef
  {
    V2mSoundDef()
    {
      ::sdInit();
    }

    ~V2mSoundDef()
    {
      ::sdClose();
    }
  };

  class V2mEngine
  {
  public:
    static const uint_t SAMPLERATE = 44100;

    static DataPtr Convert(Binary::View in)
    {
      static const V2mSoundDef SOUNDDEF;
      uint8_t* outData = nullptr;
      int outSize = 0;
      ::ConvertV2M(static_cast<const uint8_t*>(in.Start()), in.Size(), &outData, &outSize);
      Require(outData != nullptr && outSize != 0);
      return DataPtr(outData, &Deleter);
    }

    explicit V2mEngine(DataPtr data)
      : Data(std::move(data))
    {
      Player.Init();
      Require(Player.Open(Data.get(), SAMPLERATE));
      Reset();
    }

    ~V2mEngine()
    {
      Player.Close();
    }

    void Reset()
    {
      Player.Play(0);
      Require(Player.IsPlaying());
    }

    Sound::Chunk RenderFrame(uint_t samples)
    {
      RenderImpl(samples);
      Sound::Chunk result(samples);
      std::transform(Buffer.data(), Buffer.data() + samples, result.data(), &ConvertSample);
      return result;
    }

    void SkipFrame(uint_t samples)
    {
      RenderImpl(samples);
    }

  private:
    void RenderImpl(uint_t samples)
    {
      Buffer.resize(std::max<uint_t>(samples, Buffer.size()));
      Player.Render(Buffer.front().data(), samples);
    }

    using FloatPair = std::array<float, 2>;

    static Sound::Sample ConvertSample(const FloatPair& input)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      return Sound::Sample(ConvertSingleSample(input[0]), ConvertSingleSample(input[1]));
    }

    static Sound::Sample::Type ConvertSingleSample(float input)
    {
      return input * 16384;
    }

    const DataPtr Data;
    std::vector<FloatPair> Buffer;
    V2MPlayer Player;
  };

  const auto FRAME_DURATION = Time::Milliseconds(100);

  uint_t GetSamples(Time::Microseconds period)
  {
    return period.Get() * V2mEngine::SAMPLERATE / period.PER_SECOND;
  }

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(DataPtr tune, Time::Milliseconds duration, Sound::Converter::Ptr target)
      : Engine(std::move(tune))
      , State(MakePtr<TimedState>(duration))
      , Target(std::move(target))
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render(const Sound::LoopParameters& looped) override
    {
      if (!State->IsValid())
      {
        return {};
      }
      const auto avail = State->Consume(FRAME_DURATION, looped);

      auto frame = Target->Apply(Engine.RenderFrame(GetSamples(avail)));
      if (State->At() < Time::AtMillisecond() + FRAME_DURATION)
      {
        Engine.Reset();
      }
      return frame;
    }

    void Reset() override
    {
      Engine.Reset();
      State->Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      if (request < State->At())
      {
        Engine.Reset();
      }
      const auto toSkip = State->Seek(request);
      for (auto samples = GetSamples(toSkip); samples != 0;)
      {
        const auto toSkip = std::min(samples, GetSamples(FRAME_DURATION));
        Engine.SkipFrame(toSkip);
        samples -= toSkip;
      }
    }

  private:
    V2mEngine Engine;
    const TimedState::Ptr State;
    const Sound::Converter::Ptr Target;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(DataPtr data, Time::Milliseconds duration, Parameters::Accessor::Ptr props)
      : Data(std::move(data))
      , Duration(duration)
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateTimedInfo(Duration);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(Data, Duration, Sound::CreateResampler(V2mEngine::SAMPLERATE, samplerate));
    }

  private:
    const DataPtr Data;
    const Time::Milliseconds Duration;
    const Parameters::Accessor::Ptr Properties;
  };

  class DataBuilder : public Formats::Chiptune::V2m::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetTotalDuration(Time::Milliseconds duration) override
    {
      Duration = duration;
    }

    Time::Milliseconds GetDuration() const
    {
      return Duration;
    }

  private:
    MetaProperties Meta;
    Time::Milliseconds Duration;
  };

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::V2m::Parse(rawData, dataBuilder))
        {
          props.SetSource(*container);
          auto data = V2mEngine::Convert(*container);
          return MakePtr<Holder>(std::move(data), dataBuilder.GetDuration(), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create V2M: {}", e.what());
      }
      return Module::Holder::Ptr();
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::V2M
