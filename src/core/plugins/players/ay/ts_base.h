/*
Abstract:
  TurboSound functionality helpers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__

//library includes
#include <core/module_holder.h>
#include <core/module_types.h>
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Module
  {
    class TSMixer : public Sound::MultichannelReceiver
    {
    public:
      typedef boost::shared_ptr<TSMixer> Ptr;

      virtual void SetStream(uint_t idx) = 0;
    };

    TrackState::Ptr CreateTSTrackState(TrackState::Ptr first, TrackState::Ptr second);
    Analyzer::Ptr CreateTSAnalyzer(Analyzer::Ptr first, Analyzer::Ptr second);
    TSMixer::Ptr CreateTSMixer(Sound::MultichannelReceiver::Ptr delegate);
    Renderer::Ptr CreateTSRenderer(Holder::Ptr first, Holder::Ptr second, Sound::MultichannelReceiver::Ptr target);
    Renderer::Ptr CreateTSRenderer(Renderer::Ptr first, Renderer::Ptr second, TSMixer::Ptr mixer);
  }
}
#endif //__CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__
