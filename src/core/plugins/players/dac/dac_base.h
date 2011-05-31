/*
Abstract:
  DAC-based players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__
#define __CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__

//library includes
#include <core/module_types.h>
#include <devices/dac.h>
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Module
  {
    namespace DAC
    {
      Devices::DAC::Receiver::Ptr CreateReceiver(Sound::MultichannelReceiver::Ptr target);
      Analyzer::Ptr CreateAnalyzer(Devices::DAC::Chip::Ptr device);
      Devices::DAC::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__
