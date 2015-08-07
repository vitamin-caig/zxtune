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
#include "duration.h"
//library includes
#include <core/plugins_parameters.h>
#include <parameters/accessor.h>
#include <sound/render_params.h>

namespace Module
{
  uint_t GetDurationInFrames(const Parameters::Accessor& params, const String& type)
  {
    const Time::Microseconds duration = GetDuration(params, type);
    const Time::Microseconds frameDuration = Sound::GetFrameDuration(params);
    return duration.Get() / frameDuration.Get();
  }
  
  Time::Seconds GetDuration(const Parameters::Accessor& params, const String& type)
  {
    using namespace Parameters::ZXTune::Core::Plugins;
    Parameters::IntType duration = DEFAULT_DURATION_DEFAULT;
    if (!params.FindValue(DEFAULT_DURATION(type), duration))
    {
      params.FindValue(DEFAULT_DURATION(String()), duration);
    }
    return Time::Seconds(duration);
  }
}
