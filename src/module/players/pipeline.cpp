/**
 *
 * @file
 *
 * @brief  Sound pipeline support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/pipeline.h"

#include "debug/log.h"
#include "module/holder.h"
#include "module/loop.h"
#include "parameters/merged_accessor.h"
#include "parameters/tracking_helper.h"
#include "sound/gainer.h"
#include "sound/render_params.h"
#include "sound/sound_parameters.h"

#include "make_ptr.h"

#include <algorithm>

namespace Module
{
  const Debug::Stream Debug("Core::Pipeline");

  using TimeUnit = Time::Millisecond;

  Time::Duration<TimeUnit> GetDurationValue(const Parameters::Accessor& params, Parameters::Identifier name,
                                            Parameters::IntType def, Parameters::IntType precision)
  {
    const auto value = Parameters::GetInteger(params, name, def);
    return Time::Duration<TimeUnit>::FromRatio(value, precision);
  }

  struct FadeInfo
  {
    const Time::Duration<TimeUnit> FadeIn;
    const Time::Duration<TimeUnit> FadeOut;
    const Time::Duration<TimeUnit> Duration;

    FadeInfo(Time::Duration<TimeUnit> fadeIn, Time::Duration<TimeUnit> fadeOut, Time::Duration<TimeUnit> duration)
      : FadeIn(fadeIn)
      , FadeOut(fadeOut)
      , Duration(duration)
    {
      Debug("Fading: {}+{}/{} ms", fadeIn.Get(), fadeOut.Get(), duration.Get());
    }

    bool IsValid() const
    {
      return FadeIn + FadeOut < Duration;
    }

    Sound::Gain::Type GetFadein(Sound::Gain::Type vol, Time::Instant<TimeUnit> pos) const
    {
      return pos.Get() < FadeIn.Get() ? vol * pos.Get() / FadeIn.Get() : vol;
    }

    Sound::Gain::Type GetFadeout(Sound::Gain::Type vol, Time::Instant<TimeUnit> pos) const
    {
      return FadeOut && Duration.Get() < FadeOut.Get() + pos.Get() ? vol * (Duration.Get() - pos.Get()) / FadeOut.Get()
                                                                   : vol;
    }

    static FadeInfo Create(Time::Milliseconds duration, const Parameters::Accessor& params)
    {
      using namespace Parameters::ZXTune::Sound;
      const auto fadeIn = GetDurationValue(params, FADEIN, FADEIN_DEFAULT, FADEIN_PRECISION);
      const auto fadeOut = GetDurationValue(params, FADEOUT, FADEOUT_DEFAULT, FADEOUT_PRECISION);
      return {fadeIn, fadeOut, duration};
    }
  };

  // Use inlined realization due to requied Reset() method
  class SilenceDetector
  {
  public:
    bool Detected(const Sound::Chunk& in)
    {
      if (in.empty() || Limit == 0)
      {
        return false;
      }
      const auto silent = LastSample;
      if (Counter != 0 && std::all_of(in.begin(), in.end(), [silent](Sound::Sample in) { return in == silent; }))
      {
        Counter += in.size();
      }
      else
      {
        LastSample = in.back();
        // approximate counting
        Counter = std::count(in.begin(), in.end(), LastSample);
      }
      return Counter >= Limit;
    }

    void Reset()
    {
      Counter = 0;
    }

    static SilenceDetector Create(uint_t samplerate, const Parameters::Accessor& params)
    {
      using namespace Parameters::ZXTune::Sound;
      const auto duration = GetDurationValue(params, SILENCE_LIMIT, SILENCE_LIMIT_DEFAULT, SILENCE_LIMIT_PRECISION);
      const auto limit = std::size_t(samplerate) * duration.Get() / duration.PER_SECOND;
      Debug("Silence detection: {} ms ({} samples)", duration.Get(), limit);
      return SilenceDetector(limit);
    }

  private:
    explicit SilenceDetector(std::size_t limit)
      : Limit(limit)
    {}

  private:
    const std::size_t Limit;
    std::size_t Counter = 0;
    Sound::Sample LastSample;
  };

  class PipelinedRenderer : public Renderer
  {
  public:
    PipelinedRenderer(const Holder& holder, uint_t samplerate, Parameters::Accessor::Ptr params)
      : Delegate(holder.CreateRenderer(samplerate, params))
      , State(Delegate->GetState())
      , Params(std::move(params))
      , Fading(FadeInfo::Create(holder.GetModuleInformation()->Duration(), *Params))
      , Gainer(Sound::CreateGainer())
      , Silence(SilenceDetector::Create(samplerate, *Params))
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render() override
    {
      UpdateParameters();
      if (!Loop(State->LoopCount()))
      {
        return {};
      }
      const auto posBefore = State->At();
      auto data = Delegate->Render();
      if (Silence.Detected(data))
      {
        return {};
      }
      Gainer->SetGain(CalculateGain(posBefore));
      return Gainer->Apply(std::move(data));
    }

    void Reset() override
    {
      Delegate->Reset();
      Params.Reset();
      Silence.Reset();
    }

    void SetPosition(Time::AtMillisecond position) override
    {
      Silence.Reset();
      Delegate->SetPosition(position);
    }

  private:
    void UpdateParameters()
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Sound;
        const auto val = Parameters::GetInteger(*Params, GAIN, GAIN_DEFAULT);
        Preamp = Sound::Gain::Type(val, GAIN_PRECISION);
        Debug("Preamp: {}%", val);
        Loop = Sound::GetLoopParameters(*Params);
      }
    }

    Sound::Gain::Type CalculateGain(Time::AtMillisecond posBefore)
    {
      if (!Fading.IsValid())
      {
        return Preamp;
      }
      // Invariants:
      // if doneLoops == 0 then always posAfter > posBefore
      const auto posAfter = State->At();
      const auto doneLoops = State->LoopCount();
      const auto lastIteration = !Loop(doneLoops + 1);

      // If looped, do not allow fadein
      // to avoid absolute silence
      const auto part1 = doneLoops ? Preamp : Fading.GetFadein(Preamp, posAfter);
      return lastIteration ? Fading.GetFadeout(part1, std::min(posBefore, posAfter)) : part1;
    }

  private:
    const Renderer::Ptr Delegate;
    const Module::State::Ptr State;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    const FadeInfo Fading;
    const Sound::Gainer::Ptr Gainer;
    SilenceDetector Silence;
    Sound::Gain::Type Preamp;
    LoopParameters Loop;
  };

  Renderer::Ptr CreatePipelinedRenderer(const Holder& holder, Parameters::Accessor::Ptr globalParams)
  {
    const auto samplerate = Sound::GetSoundFrequency(*globalParams);
    return CreatePipelinedRenderer(holder, samplerate, std::move(globalParams));
  }

  Renderer::Ptr CreatePipelinedRenderer(const Holder& holder, uint_t samplerate, Parameters::Accessor::Ptr globalParams)
  {
    auto props = Parameters::CreateMergedAccessor(holder.GetModuleProperties(), std::move(globalParams));
    return MakePtr<PipelinedRenderer>(holder, samplerate, std::move(props));
  }
}  // namespace Module
