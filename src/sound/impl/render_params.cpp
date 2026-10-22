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

#include "sound/parameters.h"

namespace Sound
{
  Module::LoopParameters GetLoopParameters(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Sound;
    return {Parameters::Get<Looped>(params), Parameters::Get<LoopLimit>(params)};
  }

  uint_t GetSoundFrequency(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Sound;
    return Parameters::Get<Frequency>(params);
  }
}  // namespace Sound
