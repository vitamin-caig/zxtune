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

namespace ZXTune
{
  namespace Module
  {
    Analyzer::Ptr CreateDACAnalyzer(DAC::Chip::Ptr device);
  }
}

#endif //__CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__
