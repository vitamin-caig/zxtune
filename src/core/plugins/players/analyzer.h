/*
Abstract:
  Analyzer helper routines

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_ANALYZER_HELPER_H_DEFINED
#define CORE_PLUGINS_PLAYERS_ANALYZER_HELPER_H_DEFINED

//library includes
#include <core/module_types.h>

namespace ZXTune
{
  namespace Module
  {
    template<class It>
    void ConvertAnalyzerState(It begin, It end, std::vector<Analyzer::ChannelState>& channels)
    {
      channels.resize(std::distance(begin, end));
      std::vector<Analyzer::ChannelState>::iterator out = channels.begin();
      for (It it = begin; it != end; ++it, ++out)
      {
        out->Enabled = it->Enabled;
        out->Band = it->Band;
        out->Level = it->Level.Raw();
      }
    }
  }
}

#endif //CORE_PLUGINS_PLAYERS_ANALYZER_HELPER_H_DEFINED
