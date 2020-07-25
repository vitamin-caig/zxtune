/**
* 
* @file
*
* @brief  Duration info implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/duration.h"
//library includes
#include <core/plugins_parameters.h>
#include <parameters/accessor.h>
#include <sound/render_params.h>

namespace Module
{
  uint_t GetDurationInFrames(const Parameters::Accessor& params)
  {
    const auto frameDuration = Sound::GetFrameDuration(params);
    const auto duration = GetDuration(params);
    return duration.Divide<uint_t>(frameDuration);
  }
  
  Time::Seconds GetDuration(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Core::Plugins;
    Parameters::IntType duration = DEFAULT_DURATION_DEFAULT;
    params.FindValue(DEFAULT_DURATION, duration);
    return Time::Seconds(duration);
  }
}
