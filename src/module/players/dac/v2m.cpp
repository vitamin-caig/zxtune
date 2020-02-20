/**
* 
* @file
*
* @brief  V2M chiptune factory implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/dac/v2m.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <formats/chiptune/digital/v2m.h>
#include <module/players/analyzer.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <parameters/tracking_helper.h>
#include <sound/render_params.h>
#include <sound/resampler.h>
#include <sound/sound_parameters.h>
//3rdparty
#include <3rdparty/v2m/src/sounddef.h>
#include <3rdparty/v2m/src/v2mconv.h>
#include <3rdparty/v2m/src/v2mplayer.h>
//text includes
#include <module/text/platforms.h>

namespace Module
{
namespace V2M
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

    Sound::Chunk RenderFrame(Time::Milliseconds duration)
    {
      const auto samples = RenderImpl(duration);
      Sound::Chunk result(samples);
      std::transform(Buffer.data(), Buffer.data() + samples, result.data(), &ConvertSample);
      return result;
    }

    void SkipFrame(Time::Milliseconds duration)
    {
      RenderImpl(duration);
    }
  private:
    uint_t RenderImpl(Time::Milliseconds duration)
    {
      const auto samples = duration.Get() * SAMPLERATE / duration.PER_SECOND;
      Buffer.resize(std::max<uint_t>(samples, Buffer.size()));
      Player.Render(Buffer.front().data(), samples);
      return samples;
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

  const auto FRAME_DURATION = Time::Milliseconds(20);

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(DataPtr tune, StateIterator::Ptr iterator, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params)
      : Engine(std::move(tune))
      , Iterator(std::move(iterator))
      , State(Iterator->GetStateObserver())
      , Target(std::move(target))
      , Analyzer(Module::CreateSoundAnalyzer())
      , SoundParams(Sound::RenderParameters::Create(std::move(params)))
      , Looped()
    {
      ApplyParameters();
    }

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Module::Analyzer::Ptr GetAnalyzer() const override
    {
      return Analyzer;
    }

    bool RenderFrame() override
    {
      try
      {
        ApplyParameters();

        auto frame = Engine.RenderFrame(FRAME_DURATION);
        Analyzer->AddSoundData(frame);
        Resampler->ApplyData(std::move(frame));
        Iterator->NextFrame(Looped);
        if (0 == State->Frame())
        {
          Engine.Reset();
        }
        return Iterator->IsValid();
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    void Reset() override
    {
      SoundParams.Reset();
      Engine.Reset();
    }

    void SetPosition(uint_t frame) override
    {
      auto current = State->Frame();
      auto& iter = *Iterator;
      if (frame <= current)
      {
        iter.Reset();
        Engine.Reset();
        current = 0;
      }
      while (current < frame && iter.IsValid())
      {
        Engine.SkipFrame(FRAME_DURATION);
        iter.NextFrame({});
        ++current;
      }
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        Looped = SoundParams->Looped();
        Resampler = Sound::CreateResampler(Engine.SAMPLERATE, SoundParams->SoundFreq(), Target);
      }
    }
  private:
    V2mEngine Engine;
    const StateIterator::Ptr Iterator;
    const Module::State::Ptr State;
    const Sound::Receiver::Ptr Target;
    const Module::SoundAnalyzer::Ptr Analyzer;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
    Sound::Receiver::Ptr Resampler;
    Sound::LoopParameters Looped;
  };
  
  class Holder : public Module::Holder
  {
  public:
    Holder(DataPtr data, Module::Information::Ptr info, Parameters::Accessor::Ptr props)
      : Data(std::move(data))
      , Info(std::move(info))
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

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return MakePtr<Renderer>(Data, Module::CreateStreamStateIterator(Info), std::move(target), std::move(params));
    }
  private:
    const DataPtr Data;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };
  
  class DataBuilder : public Formats::Chiptune::V2m::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Meta(props)
    {
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetTotalDuration(Time::Milliseconds duration) override
    {
      FramesCount = duration.Divide<uint_t>(FRAME_DURATION);
    }

    uint_t GetFramesCount() const
    {
      return FramesCount;
    }
  private:
    MetaProperties Meta;
    uint_t FramesCount = 0;
  };
  
  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::V2m::Parse(rawData, dataBuilder))
        {
          props.SetSource(*container);
          auto data = V2mEngine::Convert(*container);
          auto info = Module::CreateStreamInfo(dataBuilder.GetFramesCount());
          return MakePtr<Holder>(std::move(data), std::move(info), std::move(properties));
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create V2M: %s", e.what());
      }
      return Module::Holder::Ptr();
    }
  };
  
  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}
}
