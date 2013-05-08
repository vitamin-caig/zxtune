/*
Abstract:
  SAA parameters players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_SAA_PARAMETERS_DEFINED
#define CORE_PLUGINS_PLAYERS_SAA_PARAMETERS_DEFINED

//local includes
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <core/module_holder.h>
#include <devices/saa.h>

namespace ZXTune
{
  namespace Module
  {
    namespace SAA
    {
      Devices::SAA::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);

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

#endif //CORE_PLUGINS_PLAYERS_SAA_PARAMETERS_DEFINED
