/**
 *
 * @file
 *
 * @brief Update parameters definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "app_parameters.h"

namespace Parameters::ZXTuneQT::Update
{
  const auto PREFIX = ZXTuneQT::PREFIX + "Update"_id;

  //@{
  //! @name Feed URL
  const auto FEED = PREFIX + "Feed"_id;
  //@}

  //@{
  //! @name Check period

  //! Parameter name
  const auto CHECK_PERIOD = PREFIX + "CheckPeriod"_id;
  //! Default value- once a day
  const IntType CHECK_PERIOD_DEFAULT = 86400;
  //@}

  //@{
  //! @name Last check time

  //! Parameter name
  const auto LAST_CHECK = PREFIX + "LastCheck"_id;
  //@}
}  // namespace Parameters::ZXTuneQT::Update
