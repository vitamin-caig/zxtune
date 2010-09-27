/*
Abstract:
  TurboSound functionality helpers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__

//library includes
#include <core/module_types.h>

namespace ZXTune
{
  namespace Module
  {
    TrackState::Ptr CreateTSTrackState(TrackState::Ptr first, TrackState::Ptr second);
    Analyzer::Ptr CreateTSAnalyzer(Analyzer::Ptr first, Analyzer::Ptr second);
  }
}
#endif //__CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__
