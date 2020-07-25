/**
*
* @file
*
* @brief  Render params implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
//library includes
#include <sound/render_params.h>
#include <sound/sound_parameters.h>

#include <utility>

namespace Sound
{
  class RenderParametersImpl : public RenderParameters
  {
  public:
    explicit RenderParametersImpl(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {
    }

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint_t SoundFreq() const override
    {
      using namespace Parameters::ZXTune::Sound;
      return static_cast<uint_t>(FoundProperty(FREQUENCY, FREQUENCY_DEFAULT));
    }

    Time::Microseconds FrameDuration() const override
    {
      using namespace Parameters::ZXTune::Sound;
      return Time::Microseconds(FoundProperty(FRAMEDURATION, FRAMEDURATION_DEFAULT));
    }

    LoopParameters Looped() const override
    {
      using namespace Parameters::ZXTune::Sound;
      return {0 != FoundProperty(LOOPED, 0), static_cast<uint_t>(FoundProperty(LOOP_LIMIT, 0))};
    }

    uint_t SamplesPerFrame() const override
    {
      const uint_t freq = SoundFreq();
      const auto frameDuration = FrameDuration();
      return static_cast<uint_t>(frameDuration.Get() * freq / frameDuration.PER_SECOND);
    }
  private:
    Parameters::IntType FoundProperty(const Parameters::NameType& name, Parameters::IntType defVal) const
    {
      Parameters::IntType ret = defVal;
      Params->FindValue(name, ret);
      return ret;
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };
}

namespace Sound
{
  RenderParameters::Ptr RenderParameters::Create(Parameters::Accessor::Ptr soundParameters)
  {
    return MakePtr<RenderParametersImpl>(soundParameters);
  }

  Time::Microseconds GetFrameDuration(const Parameters::Accessor& params)
  {
    Parameters::IntType value = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
    params.FindValue(Parameters::ZXTune::Sound::FRAMEDURATION, value);
    return Time::Microseconds(value);
  }

  void SetFrameDuration(Parameters::Modifier& params, Time::Microseconds duration)
  {
    params.SetValue(Parameters::ZXTune::Sound::FRAMEDURATION, duration.Get());
  }
}
