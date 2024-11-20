/**
 *
 * @file
 *
 * @brief  Duration info implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/duration.h"

#include "core/plugins_parameters.h"
#include "parameters/accessor.h"

namespace Module
{
  Time::Seconds GetDefaultDuration(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Core::Plugins;
    const auto duration = Parameters::GetInteger<uint_t>(params, DEFAULT_DURATION, DEFAULT_DURATION_DEFAULT);
    return Time::Seconds{duration};
  }
}  // namespace Module
