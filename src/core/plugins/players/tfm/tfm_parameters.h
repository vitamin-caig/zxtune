/*
Abstract:
  TFM parameters players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TFM_PARAMETERS_DEFINED
#define CORE_PLUGINS_PLAYERS_TFM_PARAMETERS_DEFINED

//local includes
#include "tfm.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <core/module_holder.h>

namespace ZXTune
{
  namespace Module
  {
    namespace TFM
    {
      Devices::TFM::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);

      class TrackParameters
      {
      public:
        typedef boost::shared_ptr<const TrackParameters> Ptr;
        virtual ~TrackParameters() {}

        virtual bool Looped() const = 0;
        virtual Time::Microseconds FrameDuration() const = 0;

        static Ptr Create(Parameters::Accessor::Ptr params);
      };
    }
  }
}

#endif //CORE_PLUGINS_PLAYERS_TFM_PARAMETERS_DEFINED
