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

//library includes
#include <devices/saa.h>
#include <parameters/accessor.h>

namespace Module
{
  namespace SAA
  {
    Devices::SAA::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);
  }
}

#endif //CORE_PLUGINS_PLAYERS_SAA_PARAMETERS_DEFINED
