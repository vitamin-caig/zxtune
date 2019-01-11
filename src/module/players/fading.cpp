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
#include "fading.h"
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
  
  struct FadeInfo
  {
    const uint_t FadeIn;
    const uint_t FadeOut;
    const uint_t Duration;
    
    FadeInfo(uint_t fadeIn, uint_t fadeOut, uint_t duration)
      : FadeIn(fadeIn)
      , FadeOut(fadeOut)
      , Duration(duration)
    {
    }
    
    bool IsValid() const
    {
      return FadeIn + FadeOut < Duration;
    }
    
    bool IsFadein(uint_t pos) const
    {
      return pos < FadeIn;
    }
    
    bool IsFadeout(uint_t pos) const
    {
      return pos > Duration - FadeOut;
    }
    
    Sound::Gain::Type GetFadein(Sound::Gain::Type vol, uint_t pos) const
    {
      return vol * pos / FadeIn;
    }

    Sound::Gain::Type GetFadeout(Sound::Gain::Type vol, uint_t pos) const
    {
      return vol * (Duration - pos) / FadeOut;
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
      const auto pos = Status->Frame();
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

  Sound::GainSource::Ptr CreateGainSource(Parameters::Accessor::Ptr params, Information::Ptr info, State::Ptr status)
  {
    using namespace Parameters::ZXTune::Sound;
    auto fadeIn = FADEIN_DEFAULT;
    auto fadeOut = FADEOUT_DEFAULT;
    params->FindValue(FADEIN, fadeIn);
    params->FindValue(FADEOUT, fadeOut);
    if (fadeIn == 0 && fadeOut == 0)
    {
      return MakePtr<SimpleGainSource>(std::move(params));
    }
    auto frameDuration = FRAMEDURATION_DEFAULT;
    params->FindValue(FRAMEDURATION, frameDuration);
    Require(frameDuration != 0);
    static_assert(FRAMEDURATION_PRECISION == FADEIN_PRECISION, "Different precision");
    static_assert(FRAMEDURATION_PRECISION == FADEOUT_PRECISION, "Different precision");
    const FadeInfo fading(fadeIn / frameDuration, fadeOut / frameDuration, info->FramesCount());
    return MakePtr<FadingGainSource>(std::move(params), fading, std::move(status));
  }
}
