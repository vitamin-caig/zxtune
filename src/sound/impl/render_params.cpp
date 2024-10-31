/**
 *
 * @file
 *
 * @brief  Render params implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/render_params.h"

#include "parameters/identifier.h"
#include "parameters/types.h"
#include "sound/sound_parameters.h"

#include "make_ptr.h"

#include <utility>

namespace Sound
{
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
      return Parameters::GetInteger<uint_t>(*Params, FREQUENCY, FREQUENCY_DEFAULT);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };
}  // namespace Sound

namespace Sound
{
  RenderParameters::Ptr RenderParameters::Create(Parameters::Accessor::Ptr soundParameters)
  {
    return MakePtr<RenderParametersImpl>(std::move(soundParameters));
  }

  Module::LoopParameters GetLoopParameters(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Sound;
    return {0 != Parameters::GetInteger(params, LOOPED), Parameters::GetInteger<uint_t>(params, LOOP_LIMIT)};
  }

  uint_t GetSoundFrequency(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Sound;
    return Parameters::GetInteger<uint_t>(params, FREQUENCY, FREQUENCY_DEFAULT);
  }
}  // namespace Sound
