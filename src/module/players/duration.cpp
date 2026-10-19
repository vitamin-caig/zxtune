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

#include "core/plugins/parameters.h"

#include "parameters/accessor.h"

namespace Module
{
  Time::Seconds GetDefaultDuration(const Parameters::Accessor& params)
  {
    using namespace Parameters::ZXTune::Core::Plugins;
    return Time::Seconds{Parameters::Get<DefaultDuration>(params)};
  }
}  // namespace Module
