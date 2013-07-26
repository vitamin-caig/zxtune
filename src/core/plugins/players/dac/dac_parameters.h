/*
Abstract:
  Parameters adapter factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_DAC_PARAMETERS_DEFINED
#define CORE_PLUGINS_PLAYERS_DAC_PARAMETERS_DEFINED

//common includes
#include <parameters.h>
//library includes
#include <devices/dac.h>

namespace Module
{
  namespace DAC
  {
    Devices::DAC::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);
  }
}

#endif //CORE_PLUGINS_PLAYERS_DAC_PARAMETERS_DEFINED
