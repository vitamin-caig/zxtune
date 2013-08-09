/*
Abstract:
  Update parameters definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UPDATE_PARAMETERS_H_DEFINED
#define ZXTUNE_QT_UPDATE_PARAMETERS_H_DEFINED

//local includes
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
    }
  }
}
#endif //ZXTUNE_QT_UPDATE_PARAMETERS_H_DEFINED
