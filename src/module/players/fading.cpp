/**
* 
* @file
*
* @brief  Fading support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/fading.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <parameters/tracking_helper.h>
#include <sound/sound_parameters.h>

namespace Module
{
  class SimpleGainSource : public Sound::GainSource
  {
  public:
    explicit SimpleGainSource(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {
    }
    
    Sound::Gain::Type Get() const override
    {
      SynchronizeParameters();
      return Value;
    }
    
  private:
    void SynchronizeParameters() const
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Sound;
        auto gain = GAIN_DEFAULT;
        Params->FindValue(GAIN, gain);
        Value = Sound::Gain::Type(gain, GAIN_PRECISION);
      }
    }
  private:
    mutable Parameters::TrackingHelper<Parameters::Accessor> Params;
    mutable Sound::Gain::Type Value;
  };

  using TimeUnit = Time::Millisecond;

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
    }
    
    bool IsValid() const
    {
      return FadeIn + FadeOut < Duration;
    }
    
    bool IsFadein(Time::Instant<TimeUnit> pos) const
    {
      return pos.Get() < FadeIn.Get();
    }
    
    bool IsFadeout(Time::Instant<TimeUnit> pos) const
    {
      return Duration.Get() < FadeOut.Get() + pos.Get();
    }
    
    Sound::Gain::Type GetFadein(Sound::Gain::Type vol, Time::Instant<TimeUnit> pos) const
    {
      return vol * pos.Get() / FadeIn.Get();
    }

    Sound::Gain::Type GetFadeout(Sound::Gain::Type vol, Time::Instant<TimeUnit> pos) const
    {
      return vol * (Duration.Get() - pos.Get()) / FadeOut.Get();
    }
  };
  
  class FadingGainSource : public Sound::GainSource
  {
  public:
    FadingGainSource(Parameters::Accessor::Ptr params, FadeInfo fading, State::Ptr status)
      : Params(std::move(params))
      , Fading(fading)
      , Status(std::move(status))
      , Looped()
    {
    }

    Sound::Gain::Type Get() const override
    {
      SynchronizeParameters();
      return Current;
    }
  private:
    void SynchronizeParameters() const
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Sound;
        auto val = GAIN_DEFAULT;
        Params->FindValue(GAIN, val);
        Maximum = Sound::Gain::Type(val, GAIN_PRECISION);
        val = 0;
        Params->FindValue(LOOPED, val);
        Looped = val;
      }
      const auto pos = Status->At();
      if (Fading.IsFadein(pos))
      {
        Current = Fading.GetFadein(Maximum, pos);
      }
      else if (Looped == 0 && Fading.IsFadeout(pos))
      {
        Current = Fading.GetFadeout(Maximum, pos);
      }
      else
      {
        Current = Maximum;
      }
    }
  private:
    mutable Parameters::TrackingHelper<Parameters::Accessor> Params;
    const FadeInfo Fading;
    const State::Ptr Status;
    mutable Sound::Gain::Type Maximum;
    mutable Sound::Gain::Type Current;
    mutable Parameters::IntType Looped;
  };

  Time::Duration<TimeUnit> Get(const Parameters::Accessor& params, const Parameters::NameType& name, Parameters::IntType def, Parameters::IntType precision)
  {
    auto value = def;
    params.FindValue(name, value);
    return Time::Duration<TimeUnit>::FromRatio(value, precision);
  }

  Sound::GainSource::Ptr CreateGainSource(Parameters::Accessor::Ptr params, Time::Milliseconds duration, State::Ptr status)
  {
    using namespace Parameters::ZXTune::Sound;
    const auto fadeIn = Get(*params, FADEIN, FADEIN_DEFAULT, FADEIN_PRECISION);
    const auto fadeOut = Get(*params, FADEOUT, FADEOUT_DEFAULT, FADEOUT_PRECISION);
    if (fadeIn.Get() == 0 && fadeOut.Get() == 0)
    {
      return MakePtr<SimpleGainSource>(std::move(params));
    }
    const FadeInfo fading(fadeIn, fadeOut, duration);
    return MakePtr<FadingGainSource>(std::move(params), fading, std::move(status));
  }
}
