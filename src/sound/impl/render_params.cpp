/**
 *
 * @file
 *
 * @brief  Render params implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
// library includes
#include <sound/render_params.h>
#include <sound/sound_parameters.h>

#include <utility>

namespace Sound
{
  Parameters::IntType GetProperty(const Parameters::Accessor& params, Parameters::Identifier name,
                                  Parameters::IntType defVal = 0)
  {
    Parameters::IntType ret = defVal;
    params.FindValue(name, ret);
    return ret;
  }

  class RenderParametersImpl : public RenderParameters
  {
  public:
    explicit RenderParametersImpl(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    uint_t Version() const override
    {
      return Params->Version();
    }

    uint_t SoundFreq() const override
    {
      using namespace Parameters::ZXTune::Sound;
      return static_cast<uint_t>(FoundProperty(FREQUENCY, FREQUENCY_DEFAULT));
    }

  private:
    Parameters::IntType FoundProperty(Parameters::Identifier name, Parameters::IntType defVal) const
    {
      return GetProperty(*Params, name, defVal);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };
}  // namespace Sound

namespace Sound
{
  RenderParameters::Ptr RenderParameters::Create(Parameters::Accessor::Ptr soundParameters)
  {
    return MakePtr<RenderParametersImpl>(soundParameters);
  }

  Module::LoopParameters GetLoopParameters(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Sound;
    return {0 != GetProperty(params, LOOPED), static_cast<uint_t>(GetProperty(params, LOOP_LIMIT))};
  }

  uint_t GetSoundFrequency(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Sound;
    return static_cast<uint_t>(GetProperty(params, FREQUENCY, FREQUENCY_DEFAULT));
  }
}  // namespace Sound
