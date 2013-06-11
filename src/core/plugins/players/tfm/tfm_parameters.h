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

//common includes
#include <parameters.h>
//library includes
#include <devices/tfm.h>

namespace ZXTune
{
  namespace Module
  {
    namespace TFM
    {
      Devices::TFM::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);
    }
  }
}

#endif //CORE_PLUGINS_PLAYERS_TFM_PARAMETERS_DEFINED
