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

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace Update
    {
      const std::string NAMESPACE_NAME("Update");

      const NameType PREFIX = ZXTuneQT::PREFIX + NAMESPACE_NAME;

      //@{
      //! @name Feed URL
      const NameType FEED = PREFIX + "Feed";
      //@}

      //@{
      //! @name Check period

      //! Parameter name
      const NameType CHECK_PERIOD = PREFIX + "CheckPeriod";
      //! Default value- once a day
      const IntType CHECK_PERIOD_DEFAULT = 86400;
      //@}

      //@{
      //! @name Last check time

      //! Parameter name
      const NameType LAST_CHECK = PREFIX + "LastCheck";
      //@}
    }  // namespace Update
  }    // namespace ZXTuneQT
}  // namespace Parameters
