/**
 *
 * @file
 *
 * @brief  Duration info implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/duration.h"
// library includes
#include <core/plugins_parameters.h>
#include <parameters/accessor.h>

namespace Module
{
  Time::Seconds GetDefaultDuration(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Core::Plugins;
    Parameters::IntType duration = DEFAULT_DURATION_DEFAULT;
    params.FindValue(DEFAULT_DURATION, duration);
    return Time::Seconds{static_cast<uint_t>(duration)};
  }
}  // namespace Module
