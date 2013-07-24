/*
Abstract:
  Analyzer adapter factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_ANALYZER_H_DEFINED
#define CORE_PLUGINS_PLAYERS_ANALYZER_H_DEFINED

//library includes
#include <core/module_types.h>
#include <devices/state.h>

namespace ZXTune
{
  namespace Module
  {
    Analyzer::Ptr CreateAnalyzer(Devices::StateSource::Ptr state);
  }
}

#endif //CORE_PLUGINS_PLAYERS_ANALYZER_H_DEFINED
