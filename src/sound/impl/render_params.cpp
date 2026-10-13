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

#include "sound/sound_parameters.h"

namespace Sound
{
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
